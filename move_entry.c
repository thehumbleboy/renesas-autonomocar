#include <move.h>
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t xBotControlQueue;  // From main thread or user header

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

void Move_bot(int left_motor_val, int right_motor_val) {
   if (left_motor_val >= 0 && right_motor_val >= 0) {
      R_IOPORT_PinWrite(&g_ioport_ctrl, In1, BSP_IO_LEVEL_HIGH);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In2, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In3, BSP_IO_LEVEL_HIGH);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In4, BSP_IO_LEVEL_LOW);
      R_GPT_DutyCycleSet(&g_timer0_ctrl, left_motor_val, GPT_IO_PIN_GTIOCA);
      R_GPT_DutyCycleSet(&g_timer1_ctrl, right_motor_val, GPT_IO_PIN_GTIOCB);
   }
   else if (left_motor_val >= 0 && right_motor_val <= 0) {
      R_IOPORT_PinWrite(&g_ioport_ctrl, In1, BSP_IO_LEVEL_HIGH);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In2, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In3, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In4, BSP_IO_LEVEL_HIGH);
      right_motor_val = -right_motor_val;
      R_GPT_DutyCycleSet(&g_timer0_ctrl, left_motor_val, GPT_IO_PIN_GTIOCA);
      R_GPT_DutyCycleSet(&g_timer1_ctrl, right_motor_val, GPT_IO_PIN_GTIOCB);
   }
   else if (left_motor_val <= 0 && right_motor_val >= 0) {
      R_IOPORT_PinWrite(&g_ioport_ctrl, In1, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In2, BSP_IO_LEVEL_HIGH);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In3, BSP_IO_LEVEL_HIGH);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In4, BSP_IO_LEVEL_LOW);
      left_motor_val = -left_motor_val;
      R_GPT_DutyCycleSet(&g_timer0_ctrl, left_motor_val, GPT_IO_PIN_GTIOCA);
      R_GPT_DutyCycleSet(&g_timer1_ctrl, right_motor_val, GPT_IO_PIN_GTIOCB);
   }
   else {
      R_IOPORT_PinWrite(&g_ioport_ctrl, In1, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In2, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In3, BSP_IO_LEVEL_LOW);
      R_IOPORT_PinWrite(&g_ioport_ctrl, In4, BSP_IO_LEVEL_LOW);
      R_GPT_DutyCycleSet(&g_timer0_ctrl, left_motor_val, GPT_IO_PIN_GTIOCA);
      R_GPT_DutyCycleSet(&g_timer1_ctrl, right_motor_val, GPT_IO_PIN_GTIOCB);
   }
}

void move_entry(void *pvParameters) {
    FSP_PARAMETER_NOT_USED(pvParameters);

    // Motor pins and PWM init as before:
    R_IOPORT_PinWrite(&g_ioport_ctrl, In1, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, In2, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, In3, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, In4, BSP_IO_LEVEL_LOW);
    R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_GPT_Open(&g_timer1_ctrl, &g_timer1_cfg);
    R_GPT_PeriodSet(&g_timer0_ctrl, 100000);
    R_GPT_PeriodSet(&g_timer1_ctrl, 100000);
    R_GPT_Start(&g_timer0_ctrl);
    R_GPT_Start(&g_timer1_ctrl);

    uint8_t IR1_out = 0, IR2_out = 0, IR3_out = 0;
    int bot_should_run = 0;
    int received_command = 0;

    while (1) {
        // Block until start/stop command arrives
        if (xQueueReceive(xBotControlQueue, &received_command, portMAX_DELAY) == pdPASS) {
            bot_should_run = received_command;
            if (!bot_should_run) { Move_bot(0, 0); continue; }
        }
        // Main active loop
        while (bot_should_run) {
            R_IOPORT_PinRead(&g_ioport_ctrl, IR1, &IR1_out);
            R_IOPORT_PinRead(&g_ioport_ctrl, IR2, &IR2_out);
            R_IOPORT_PinRead(&g_ioport_ctrl, IR3, &IR3_out);

            if (IR1_out == 1) Move_bot(10000, 10000);
            else if (IR1_out == 0 && IR2_out == 1 && IR3_out == 1) {
                Move_bot(0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                Move_bot(-30000, -30000);
                vTaskDelay(pdMS_TO_TICKS(500));
                Move_bot(30000, -30000);
                vTaskDelay(pdMS_TO_TICKS(375));
            }
            else if (IR1_out == 0 && IR2_out == 1) {
                Move_bot(0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                Move_bot(-30000, -30000);
                vTaskDelay(pdMS_TO_TICKS(500));
                Move_bot(30000, -30000);
                vTaskDelay(pdMS_TO_TICKS(375));
            }
            else if (IR1_out == 0 && IR2_out == 0 && IR3_out == 1) {
                Move_bot(0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                Move_bot(-30000, -30000);
                vTaskDelay(pdMS_TO_TICKS(500));
                Move_bot(-30000, 30000);
                vTaskDelay(pdMS_TO_TICKS(375));
            }
            else {
                Move_bot(0, 0);
            }
            vTaskDelay(pdMS_TO_TICKS(10));

            // Non-blocking receive to check for new stop/start command
            if (xQueueReceive(xBotControlQueue, &received_command, 0) == pdPASS) {
                bot_should_run = received_command;
                if (!bot_should_run) { Move_bot(0, 0); break; }
            }
        }
    }
}
