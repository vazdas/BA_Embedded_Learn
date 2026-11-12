#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <i2c.h>
#include "at24c32.h"
#include "usart.h"
#include "logger.h"
#include "system_events.h"

extern "C" void main_cpp() {
    AT24C32_Init(&hi2c1);
    Logger_Init();

    //Logger_ClearLogs(&huart1);

    // The loop just to fill some slots for example
    /*for(int i=SYSTEM_BOOT; i<ERR_FAILED_TO_READ_MEMORY; i++) {
        Logger_AddLog(GetEventString((SystemEvent_t)i));
    }*/

    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n--- WRITING LOGS ---\r\n", 24, 100);

    // Output the all logs
    Logger_PrintAll(&huart1);

    // Output Log by number
    Logger_PrintLogByNumber(&huart1, 3);

    static uint32_t last_mainCheck_time = 0;
    const uint32_t mainCheck_interval = 300000; // For checking the system status each 5 min.

    // Variables for demonstration
    static bool check_motor_status = false;
    static bool check_system_status = false;

    while (1)
    {
        // Polling for status
        if (!check_motor_status) {
            Logger_AddLog(GetEventString(ERR_MOTOR_JAMMED));
            check_motor_status = true; // Assume that we repair the motor.
        }

        if (!check_system_status) {
            Logger_AddLog(GetEventString(ERR_STATUS_FALSE));
            check_system_status = true;
        }

        // The main polling of the current system status for general statistic
        uint32_t current_time = HAL_GetTick();
        if (current_time - last_mainCheck_time >= mainCheck_interval) {
            last_mainCheck_time = current_time;

            if (!check_system_status) {
                Logger_AddLog(GetEventString(ERR_STATUS_FALSE));
            } else {
                Logger_AddLog(GetEventString(STATUS_OK));
            }
        }
    }
}

