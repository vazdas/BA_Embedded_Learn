#include "led.h"
#include "esp_timer.h"

esp_err_t led_init(led_t *led, gpio_num_t pin) {
    if (led == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    led->pin = pin;
    led->mode = LED_MODE_OFF;
    led->current_state = false;
    led->last_toggle_time = 0;
    led->blink_interval = 500;

    gpio_reset_pin(pin);

    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);

    return ESP_OK;
}

esp_err_t led_set_mode(led_t *led, led_mode_t mode, uint32_t interval_ms) {
    if (led == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    led->mode = mode;
    led->blink_interval = interval_ms;

    if (mode == LED_MODE_ON) {
        led->current_state = true;
        gpio_set_level(led->pin, 1);
    } else if (mode == LED_MODE_OFF) {
        led->current_state = false;
        gpio_set_level(led->pin, 0);
    }

    return ESP_OK;
}

esp_err_t led_update(led_t *led) {
    if (led == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    if (led->mode != LED_MODE_BLINK) {
        return ESP_OK;
    }

    uint32_t current_time = esp_timer_get_time() / 1000;

    if (current_time - led->last_toggle_time >= led->blink_interval) {
        led->current_state = !led->current_state;
        gpio_set_level(led->pin, led->current_state);
        led->last_toggle_time = current_time;
    }

    return ESP_OK;
}
