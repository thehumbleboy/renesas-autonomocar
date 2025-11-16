
#include "hal_data.h"
#include "main_thread.h"
#include "user.h"
#include "common_utils.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "log_info.h"

#define WIDTH_64             (64)
#define CONNECT_TIMEOUT       (5000)
#define CTRL_STATE_START      (1)
#define CTRL_STATE_STOP       (0)
#define BOT_CONTROL_QUEUE_LEN 2
extern StaticQueue_t xBotControlQueueBuffer;
extern int xBotControlQueueStorage[BOT_CONTROL_QUEUE_LEN];
extern QueueHandle_t xBotControlQueue;

volatile int bot_should_run = 0;
mqtt_onchip_da16xxx_instance_ctrl_t g_rm_mqtt_onchip_da16xxx_instance;
/* Wi-Fi and MQTT config */
static const WIFINetworkParams_t net_params = {
    .ucChannel                  = 0,
    .xPassword.xWPA.cPassphrase = WIFI_PWD,
    .ucSSID                     = WIFI_SSID,
    .xPassword.xWPA.ucLength    = sizeof(WIFI_PWD),
    .ucSSIDLength               = sizeof(WIFI_SSID),
    .xSecurity                  = eWiFiSecurityWPA2,
};

static char g_led_topic[WIDTH_64] = USER_LED_TOPIC;

/* MQTT callback for subscribe events: handles LED ON/OFF and bot control commands */
void mqtt0_callback(mqtt_onchip_da16xxx_callback_args_t *p_args)
{
    if (strstr(p_args->p_topic, USER_LED_TOPIC)) {
        int led_state = strtol((char *) p_args->p_data, NULL, 10);
        log_info("Received LED command: %s", (char *) p_args->p_data);
        utils_set_LED(GREEN_LED, (uint8_t)led_state);
    }
    else if (strstr(p_args->p_topic, CONTROL_TOPIC)) {
        int bot_cmd = -1;
        if (strncmp((char *)p_args->p_data, "start", 5) == 0) {
            bot_cmd = CTRL_STATE_START;
            bot_should_run = 1;
            log_info("Received command to START Move_bot");
        } else if (strncmp((char *)p_args->p_data, "stop", 4) == 0) {
            bot_cmd = CTRL_STATE_STOP;
            bot_should_run = 0;
            log_info("Received command to STOP Move_bot");
        }
        if ((bot_cmd == CTRL_STATE_START || bot_cmd == CTRL_STATE_STOP) && xBotControlQueue != NULL) {
            // Use xQueueSend if not in ISR, or xQueueSendFromISR if inside ISR
            xQueueSend(xBotControlQueue, &bot_cmd, 0);
        }
    }
}

void main_thread_entry(void *pvParameters) {
    FSP_PARAMETER_NOT_USED(pvParameters);

    fsp_err_t result;

    // ---- STATIC QUEUE CREATION ----
    if (xBotControlQueue == NULL) {
        xBotControlQueue = xQueueCreateStatic(
            BOT_CONTROL_QUEUE_LEN,
            sizeof(int),
            (uint8_t*)xBotControlQueueStorage,
            &xBotControlQueueBuffer
        );
        configASSERT(xBotControlQueue != NULL);
    }

    log_info("Starting Wi-Fi connection...");
    if (eWiFiSuccess != WIFI_On()) {
        log_error("Wi-Fi Open failed"); utils_halt_func();
    }
    if (eWiFiSuccess != WIFI_ConnectAP(&net_params)) {
        log_error("Wi-Fi Connect failed"); utils_halt_func();
    }
    log_info("Wi-Fi connection successful!");

    /* MQTT setup */
    mqtt_onchip_da16xxx_cfg_t mqtt_cfg = g_mqtt_onchip_da16xxx_cfg;
    mqtt_cfg.p_host_name      = (char*)EXAMPLE_MQTT_HOST;
    mqtt_cfg.p_mqtt_user_name = NULL;
    mqtt_cfg.p_mqtt_password  = NULL;
    mqtt_cfg.p_callback       = mqtt0_callback;
    mqtt_cfg.mqtt_port        = 1883;

    result = RM_MQTT_DA16XXX_Open(&g_rm_mqtt_onchip_da16xxx_instance, &mqtt_cfg);
    if (FSP_SUCCESS != result) {
        log_error("MQTT Open failed"); utils_halt_func();
    }
    log_info("MQTT setup successful!");

    /* Subscribe to LED topic */
    mqtt_onchip_da16xxx_sub_info_t sub_info;
    sub_info.qos = MQTT_ONCHIP_DA16XXX_QOS_0;
    sub_info.p_topic_filter = g_led_topic;
    sub_info.topic_filter_length = (uint16_t)strlen(g_led_topic);

    result = RM_MQTT_DA16XXX_Subscribe(&g_rm_mqtt_onchip_da16xxx_instance, &sub_info, 1);
    if (FSP_SUCCESS != result) {
        log_error("MQTT Subscribe failed"); utils_halt_func();
    }
    log_info("Subscribed to LED topic: %s", g_led_topic);

    /*Subscribe to Control Topic */
    mqtt_onchip_da16xxx_sub_info_t ctrl_sub_info;
    ctrl_sub_info.qos = MQTT_ONCHIP_DA16XXX_QOS_0;
    ctrl_sub_info.p_topic_filter = CONTROL_TOPIC;
    ctrl_sub_info.topic_filter_length = (uint16_t)strlen(CONTROL_TOPIC);
    result = RM_MQTT_DA16XXX_Subscribe(&g_rm_mqtt_onchip_da16xxx_instance, &ctrl_sub_info, 1);
    if (FSP_SUCCESS != result) {
        log_error("MQTT Subscribe (control) failed"); utils_halt_func();
    }
    log_info("Subscribed to CONTROL topic: %s", CONTROL_TOPIC);

    /* Connect to broker */
    uint32_t num_retry = 3;
    while (num_retry--) {
        result = RM_MQTT_DA16XXX_Connect(&g_rm_mqtt_onchip_da16xxx_instance, CONNECT_TIMEOUT);
        if (FSP_SUCCESS == result) {break;}
    }
    if (FSP_SUCCESS != result) {
        utils_halt_func();
    }
    log_info("MQTT Connected!");

    while (1) {
        RM_MQTT_DA16XXX_Receive(&g_rm_mqtt_onchip_da16xxx_instance, &mqtt_cfg);
        vTaskDelay(100);
    }
}
