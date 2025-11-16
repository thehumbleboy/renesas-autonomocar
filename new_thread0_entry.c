#include "new_thread0.h"

#include "FreeRTOS.h"
#include "queue.h"
/* New Thread entry function */

#define BOT_CONTROL_QUEUE_LEN 2
QueueHandle_t xBotControlQueue = NULL;
StaticQueue_t xBotControlQueueBuffer;
int xBotControlQueueStorage[BOT_CONTROL_QUEUE_LEN];

void new_thread0_entry(void *pvParameters) {
    FSP_PARAMETER_NOT_USED(pvParameters);

    xBotControlQueue = xQueueCreateStatic(
        BOT_CONTROL_QUEUE_LEN,
        sizeof(int),
        (uint8_t *)xBotControlQueueStorage,
        &xBotControlQueueBuffer
    );
    configASSERT(xBotControlQueue != NULL);
    // You may want to wait here for confirmation from other threads, etc.
    vTaskDelete(NULL); // Done after init!
}
