#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "eeprom.h"
#include "adc.h"

#define LED_OUT GPIO_PIN_13
#define PortC GPIOC

extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;

// printf to UART
extern "C" int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

extern "C" void main_cpp() {
    printf("The Light Sensor is started!\n");
    if (HAL_I2C_IsDeviceReady(&hi2c1, 0xA0, 3, 100) == HAL_OK) {
        printf("EEPROM FOUND!\r\n");
    } else {
        printf("EEPROM DEAD (Check wiring/power)\r\n");
    }
    printf("Type the 'W' to write data or 'R' to read tha last data.\n");

    uint32_t previous_millis = 0;
    uint32_t led_previous_millis = 0;

    while (1)
    {
        uint32_t current_millis = HAL_GetTick();

        // Getting ADC data from LDR each 1sec.(1000ms)
        if (current_millis - previous_millis >= 1000) {
            previous_millis = current_millis;
            HAL_ADC_Start_IT(&hadc1);
        }

        // LED blinking (each 200ms)
        if (current_millis - led_previous_millis >=200) {
            led_previous_millis = current_millis;
            HAL_GPIO_TogglePin(PortC, LED_OUT);
        }

        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
            char rx_byte;
            HAL_UART_Receive(&huart1, (uint8_t*)&rx_byte, 1, 0);

            if (rx_byte == 'W' || rx_byte == 'w') {
                uint16_t current_adc = ADC_Get_Latest_Value();
                EEPROM_Write_uint16(current_adc);
            }
            else if (rx_byte == 'R' || rx_byte == 'r') {
                uint16_t last_saved_value = EEPROM_Read_uint16();

            }

        }

    }

}
