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
            uint32_t big_value = 0xAABBCCDD;
            uint8_t tx_buffer[4];

            tx_buffer[0] = (big_value >> 24) & 0xFF;
            tx_buffer[1] = (big_value >> 16) & 0xFF;
            tx_buffer[2] = (big_value >> 8) & 0xFF;
            tx_buffer[3] = big_value & 0xFF;


            UART_Transmit(&huart1, tx_buffer, 4, 100);
            printf("STM32 sent 32-bit value 0x%08X to ESP32\n", (unsigned long)big_value);
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
