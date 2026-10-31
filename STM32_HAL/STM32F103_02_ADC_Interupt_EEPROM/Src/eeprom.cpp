#include "eeprom.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

void EEPROM_Write_uint16(uint16_t data) {
    printf("\r\n[EEPROM] Write value %u.... \r\n", data);

    if (HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, MEM_ADDR, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&data, 2, 100) == HAL_OK) {
        HAL_Delay(5);
        printf("[EEPROM] Written successfully!\r\n");
    } else {
        printf("[EEPROM] ERROR Writing\r\n");
    }
}

uint16_t EEPROM_Read_uint16(void) {
    uint16_t read_value = 0;
    printf("\r\n[EEPROM] Read value...\r\n");

    if (HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, MEM_ADDR, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&read_value, 2, 100) == HAL_OK) {
        printf("[EEPROM] The readed value: %u\r\n", read_value);
    } else {
        printf("[EEPROM] ERROR reading!\r\n");
    }

    return 0;
}
