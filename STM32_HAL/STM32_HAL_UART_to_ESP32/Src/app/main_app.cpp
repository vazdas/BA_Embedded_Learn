#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "uart/uart.h"

static UART_HandleTypeDef huart1;

extern "C" void main_cpp()
{
    uint8_t tx_byte = 'T';
    uint8_t rx_byte = 0;
    bool last_button_state = false;

    HAL_StatusTypeDef status = UART_Init(&huart1, USART1, 115200);
    if (status != HAL_OK)
    {
        printf("[UART] USART1 initialization failed (status=%d)\n", (int)status);
        Error_Handler();
    }

    printf("[UART] USART1 initialized: 115200 baud, 8N1\n");

    while (1)
    {
        bool current_button_state = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET);

        if (current_button_state == true && last_button_state == false) {
            UART_Transmit(&huart1, &tx_byte, 1, 100);
            printf("STM32 sent command 'T' to ESP32\n");
        }
        last_button_state = current_button_state;

        status = UART_Receive(&huart1, &rx_byte, 1, 0);
        if (status == HAL_OK)
        {
            if (rx_byte == 'T') {
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                printf("STM32 Received 'T', the LED should toggle\n");
            }
        }
        else if (status != HAL_TIMEOUT)
        {
            printf("[UART] Receive failed (status=%d)\r\n", (int)status);
            Error_Handler();
        }

        HAL_Delay(10);
    }
}
