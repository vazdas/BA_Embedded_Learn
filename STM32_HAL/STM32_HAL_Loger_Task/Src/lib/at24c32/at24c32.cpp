#include "at24c32.h"

static I2C_HandleTypeDef *eeprom_i2c = nullptr;

void AT24C32_Init(I2C_HandleTypeDef *hi2c) {
    eeprom_i2c = hi2c;
}

HAL_StatusTypeDef AT24C32_WritePage(uint16_t mem_address, const uint8_t *data) {
    if (eeprom_i2c == nullptr) return HAL_ERROR;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        eeprom_i2c,
        AT24C32_I2C_ADDR,
        mem_address,
        I2C_MEMADD_SIZE_16BIT,
        (uint8_t*)data,
        AT24C32_PAGE_SIZE,
        HAL_MAX_DELAY);

    HAL_Delay(5);

    return status;
}

HAL_StatusTypeDef AT24C32_ReadPage(uint16_t mem_address, uint8_t *data) {
    if (eeprom_i2c == nullptr) return HAL_ERROR;

    return HAL_I2C_Mem_Read(
        eeprom_i2c,
        AT24C32_I2C_ADDR,
        mem_address,
        I2C_MEMADD_SIZE_16BIT,
        data,
        AT24C32_PAGE_SIZE,
        HAL_MAX_DELAY);
}
