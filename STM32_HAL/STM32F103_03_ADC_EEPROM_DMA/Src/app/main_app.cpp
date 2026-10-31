#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "eeprom.h"
#include "adc.h"
#include "i2c.h"

#define LED_OUT GPIO_PIN_13
#define PortC GPIOC

extern UART_HandleTypeDef huart1;

char text_buffer[100];

extern "C" void main_cpp() {
    // Launche the ADC in DMA mode just once
    ADC_Start_DMA_Mode();

    // Sending the initial message via DMA
    if (HAL_I2C_IsDeviceReady(&hi2c1, 0xA0, 3, 100) == HAL_OK) {
        sprintf(text_buffer, "\r\n--- SYSTEM REBOOT ---\r\nStart with DMA!\r\nEEPROM FOUND!\r\nType 'W' to write or 'R' to read\r\n");
    } else {
        sprintf(text_buffer, "\r\n--- SYSTEM REBOOT ---\r\nStart with DMA!\r\nEEPROM DEAD!\r\nType 'W' to write or 'R' to read\r\n");
    }

    // Відправляємо його одним цілим шматком
    HAL_UART_Transmit_DMA(&huart1, (uint8_t*)text_buffer, strlen(text_buffer));

    uint32_t previous_millis = 0;
    uint32_t led_previous_millis = 0;

    while (1)
    {
        uint32_t current_millis = HAL_GetTick();

        // Getting ADC data from LDR each 1sec.(1000ms)
        if (current_millis - previous_millis >= 1000) {
            previous_millis = current_millis;

            // Get tha latest value DMA put to the memory
            uint16_t current_adc = ADC_Get_Latest_Value();

            // Print the result of operation
            sprintf(text_buffer, "[DMA Telemetry] ADC: %u\r", current_adc);
            HAL_UART_Transmit_DMA(&huart1, (uint8_t*)text_buffer, strlen(text_buffer));
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

                sprintf(text_buffer, "\r=> Saved %u to EEPROM\r\n", current_adc);
                HAL_UART_Transmit_DMA(&huart1, (uint8_t*)text_buffer, strlen(text_buffer));
            }
            else if (rx_byte == 'R' || rx_byte == 'r') {
                uint16_t last_saved_value = EEPROM_Read_uint16();

                sprintf(text_buffer, "\r=> Read %u from EEPROM\r\n", last_saved_value);
                HAL_UART_Transmit_DMA(&huart1, (uint8_t*)text_buffer, strlen(text_buffer));
            }

        }

    }

}
