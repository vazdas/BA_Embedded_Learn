#ifndef AT24C32_H
#define AT24C32_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define AT24C32_I2C_ADDR 0xA0
#define AT24C32_PAGE_SIZE 32

// Driver initialization
void AT24C32_Init(I2C_HandleTypeDef *hi2c);

// Writing function: one page - 32 bytes
HAL_StatusTypeDef AT24C32_WritePage(uint16_t mem_address, const uint8_t *data);

// Reading function: one page - 32 bytes
HAL_StatusTypeDef AT24C32_ReadPage(uint16_t mem_address, uint8_t *data);

#endif
