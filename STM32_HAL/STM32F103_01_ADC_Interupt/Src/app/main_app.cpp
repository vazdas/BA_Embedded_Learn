#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#define LED_OUT GPIO_PIN_13
#define PortC GPIOC
volatile uint16_t adc_value = 0;
volatile uint8_t adc_data_ready = 0;
uint32_t previous_millis = 0;

extern UART_HandleTypeDef huart1;

extern "C" int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc->Instance == ADC1) {
    adc_value = HAL_ADC_GetValue(hadc);
    adc_data_ready = 1;
  }
}

extern "C" void main_cpp() {
    HAL_GPIO_WritePin(PortC, LED_OUT, GPIO_PIN_RESET);

    uint32_t current_led;

    while (1)
    {
        uint32_t current_millis = HAL_GetTick();

        if (current_millis - previous_millis >= 1000) {
            previous_millis = current_millis;
            HAL_ADC_Start_IT(&hadc1);
        }

        if (adc_data_ready) {
            adc_data_ready = 0;

            HAL_GPIO_TogglePin(PortC, LED_OUT);

            current_led = HAL_GPIO_ReadPin(PortC, LED_OUT);
            printf("The ADC value is %u | The LED current state is %u\r\n", adc_value, current_led);
        }

    }

}
