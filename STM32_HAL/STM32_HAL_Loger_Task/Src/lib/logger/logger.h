#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include "at24c32.h"
#include "usart.h"

void Logger_Init(void);

void Logger_AddLog(const char* message);
void Logger_PrintAll(UART_HandleTypeDef *huart1);
void Logger_PrintLogByNumber(UART_HandleTypeDef *huart1, uint16_t target_num);
void Logger_ClearLogs(UART_HandleTypeDef *huart1);

#endif
