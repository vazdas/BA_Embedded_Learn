#include "logger.h"
#include <stdio.h>
#include <string.h>

static uint16_t current_log_number = 0;
static uint8_t current_slot = 0;

void Logger_Init(void) {
    char buffer[32];
    int prev_num = -1;
    bool found_break = false;

    // Scanning all 128 slots of memory
    for (uint8_t i = 0; i < 128; i++) {
        memset(buffer, 0, 32);
        AT24C32_ReadPage(i * 32, (uint8_t*)buffer);

        if (buffer[0] != '#') {
            current_slot = i;
            current_log_number = (prev_num == -1) ? 0 : (prev_num + 1) % 1000;
            found_break = true;
            break;
        }

        // Get the number of current log from the text
        int current_num;
        if (sscanf(buffer, "#%d", &current_num) == 1) {
            if (prev_num != -1) {
                int expected_num = (prev_num + 1) % 1000;

                if (current_num != expected_num) {
                    current_slot = i;
                    current_log_number = expected_num;
                    found_break = true;
                    break;
                }
            }
            prev_num = current_num;
        }
    }
    if (!found_break) {
        current_slot = 0;
        current_log_number = (prev_num == -1) ? 0 : (prev_num + 1) % 1000;
    }
}

void Logger_AddLog(const char* message) {
    char buffer[32] = {0};

    // Constructing a log line
    snprintf(buffer, 32, "#%u %s", current_log_number, message);

    // Calculate the physical 16-bit address in EEPROM memory
    uint16_t mem_address = current_slot * 32;

    // Writing 32 byte via our driver
    AT24C32_WritePage(mem_address, (uint8_t*)buffer);

    // Update the counters for the next record
    current_log_number++;
    if (current_log_number > 999) {
        // Jumping back to the beginning
        current_log_number = 0;
    }

    current_slot++;
    if (current_slot > 127) {
        // Ring Buffer: rewriting the old records starting from the 0 slot after reaching the 128th record
        current_slot = 0;
    }
}

void Logger_PrintAll(UART_HandleTypeDef *huart1) {
    char buffer[32];
    char text_buffer[64];

    sprintf(text_buffer, "-- All Logs(Oldest to Newest) --\r\n");
    HAL_UART_Transmit(huart1, (uint8_t*)text_buffer, strlen(text_buffer), 100);

    // Read all 128 slots starting from the oldest
    for (int i = 0; i < 128; i++) {
        uint8_t slot_to_read = (current_slot + i) % 128;
        memset(buffer, 0, 32);
        AT24C32_ReadPage(slot_to_read * 32, (uint8_t*)buffer);

        if (buffer[0] == '#') {
            sprintf(text_buffer, "%s\r\n", buffer);
            HAL_UART_Transmit(huart1, (uint8_t*)text_buffer, strlen(text_buffer), 100);
        }
    }
}

void Logger_PrintLogByNumber(UART_HandleTypeDef *huart1, uint16_t target_num) {
    char buffer[32];
    char text_buffer[64];
    int read_num;

    for (uint8_t i = 0; i < 128; i++) {
        memset(buffer, 0, 32);
        AT24C32_ReadPage(i * 32, (uint8_t*)buffer);

        if (sscanf(buffer, "#%d", &read_num) == 1 && read_num == target_num) {
            sprintf(text_buffer, "Found Log: %s\r\n", buffer);
            HAL_UART_Transmit(huart1, (uint8_t*)text_buffer, strlen(text_buffer), 100);
            return;
        }
    }
    HAL_UART_Transmit(huart1, (uint8_t*)"Log not found.\r\n", 16, 100);
}

void Logger_ClearLogs(UART_HandleTypeDef *huart1) {
    const char* msg_start = "\r\n-- Erasing EEPROM... PLEASE WAIT --\r\n";
    HAL_UART_Transmit(huart1, (uint8_t*)msg_start, strlen(msg_start), 100);

    // Create the empty page, filled with 0xFF
    uint8_t blank_page[32];
    memset(blank_page, 0xFF, 32);

    // Clear all 128 slots
    for (int i = 0; i < 128; i++) {
        uint16_t address = i * 32;
        AT24C32_WritePage(address, blank_page);

        // Just outputting the progress
        char text_buffer[32] = {0};
        sprintf(text_buffer, "Erased slot %d\r\n", i);
        HAL_UART_Transmit(huart1, (uint8_t*)text_buffer, strlen(text_buffer), 100);
    }

    const char* msg_end = "\r\n-- EEPROM Fully Erased! --\r\n";
    HAL_UART_Transmit(huart1, (uint8_t*)msg_end, strlen(msg_end), 100);
}
