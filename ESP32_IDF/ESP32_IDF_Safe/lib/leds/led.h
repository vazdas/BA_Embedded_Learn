#ifndef LED_H
#define LED_H

#include "driver/gpio.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_BLINK
} led_mode_t;

typedef struct {
    gpio_num_t pin;
    led_mode_t mode;
    bool current_state;
    uint32_t last_toggle_time;
    uint32_t blink_interval;
} led_t;

esp_err_t led_init(led_t *led, gpio_num_t pin);
esp_err_t led_set_mode(led_t *led, led_mode_t mode, uint32_t interval_ms);

esp_err_t led_update(led_t *led);

#endif
