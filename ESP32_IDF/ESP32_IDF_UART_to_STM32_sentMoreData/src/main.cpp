#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart.h"
#include "driver/gpio.h"

static const char *TAG = "app";

extern "C" void app_main()
{
    const esp_err_t init_err = uart_init();
    if (init_err != ESP_OK) {
        ESP_LOGE(TAG, "UART initialization failed: %s", esp_err_to_name(init_err));
        return;
    }

    ESP_LOGI(TAG, "UART echo application started");

    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);

    uint8_t tx_byte = 'T';
    uint8_t rx_byte = 0;
    bool last_button_state = false;

    while (1) {
        bool current_button_state = (gpio_get_level(GPIO_NUM_0) == 0);

        if (current_button_state == true && last_button_state == false) {
            uart_transmit(&tx_byte, 1);
            ESP_LOGI(TAG, "ESP32 sent command 'T' to STM32");
        }
        last_button_state = current_button_state;

        uint8_t rx_buffer[4] = {0};

        const esp_err_t receive_err = uart_receive(rx_buffer, 4, 20);

        if (receive_err == ESP_OK) {
            uint32_t received_value = (rx_buffer[0] << 24) | (rx_buffer[1] << 16) | (rx_buffer[2] << 8) | rx_buffer[3];

            ESP_LOGI(TAG, "Combined 32-bit value is 0x%08lX", (unsigned long)received_value);

            const esp_err_t transmit_err = uart_transmit(&rx_byte, 1);
            if (transmit_err != ESP_OK) {
                ESP_LOGE(TAG, "UART transmit error: %s", esp_err_to_name(transmit_err));
            }
        } else if (receive_err != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "UART receive error: %s", esp_err_to_name(receive_err));
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
