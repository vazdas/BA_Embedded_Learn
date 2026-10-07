#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "encoder.h"
#include "driver/ledc.h"
#include "led.h"

constexpr gpio_num_t ENCODER_BUTTON_INPUT = GPIO_NUM_15;
constexpr gpio_num_t ENCODER_B_INPUT = GPIO_NUM_16;
constexpr gpio_num_t ENCODER_A_INPUT = GPIO_NUM_17;
constexpr gpio_num_t SERVO_OUTPUT = GPIO_NUM_18;
constexpr uint32_t SERVO_ANGLE_0 = 205;
constexpr uint32_t SERVO_ANGLE_180 = 1024;
constexpr uint32_t BLINKING_INTERVAL = 300;
constexpr uint16_t ENCODER_DEBOUNCE_NS = 1000;

// Making an array of LED indicators
typedef enum {
    LED_GREEN = 0,
    LED_YELLOW_1,
    LED_YELLOW_2,
    LED_YELLOW_3,
    LED_RED,
    LED_COUNT
} led_index_t;

led_t leds[LED_COUNT];

// Assigning the GPIOs for an array of LED indicators
constexpr gpio_num_t LED_PINS[LED_COUNT] = {
    GPIO_NUM_38,
    GPIO_NUM_39,
    GPIO_NUM_40,
    GPIO_NUM_41,
    GPIO_NUM_42
};

// States for the encoder's FSM
typedef enum {
    STATE_ENTER_DIGIT_1,
    STATE_ENTER_DIGIT_2,
    STATE_ENTER_DIGIT_3,
    STATE_CHECK_PASSWORD,
    STATE_UNLOCKED
} safe_state_t;

// The function for applying the encoder's states to LED indicators
void apply_leds_state(safe_state_t state);

extern "C" void app_main(void) {
    // Setting up a timer for driving a servo
    ledc_timer_config_t timer_conf = {};
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_13_BIT;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.freq_hz = 50;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Configuring a channel for the servo
    ledc_channel_config_t servo_channel_conf = {};
    servo_channel_conf.gpio_num = SERVO_OUTPUT;
    servo_channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    servo_channel_conf.channel = LEDC_CHANNEL_0;
    servo_channel_conf.timer_sel = LEDC_TIMER_0;
    servo_channel_conf.duty = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&servo_channel_conf));

    // Initializing the array of LED indicators
    for (int i = 0; i < LED_COUNT; i++) {
        led_init(&leds[i], LED_PINS[i]);
    }

    encoder_ctx_t encoder;
    int32_t last_pulses = 0;
    bool last_button_pressed = false;

    // Default state for the encoder
    safe_state_t current_state = STATE_ENTER_DIGIT_1;

    // The main password
    int passcode[3] = {3, 1, 5};

    // The array to store the user's guess
    int entered_code[3] = {0, 0, 0};

    ESP_ERROR_CHECK(encoder_init(&encoder,
                                ENCODER_A_INPUT,
                                ENCODER_B_INPUT,
                                ENCODER_BUTTON_INPUT,
                                ENCODER_DEBOUNCE_NS));

    // Variable to hold the current entered digit by the user
    int current_digit_index = 1;

    // Setting the default state for LED's indicators
    apply_leds_state(current_state);

    // Setting the servo to the default(close) position
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_ANGLE_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // The Greeting
    printf("\n=== SAFE LOCKED ===\nEnter digits from 1 to 9, press the button to apply\n Enter Digit 1:\n");

    while (1) {
        int32_t pulses = 0;
        bool button_pressed = false;

        // Reading the encoder
        ESP_ERROR_CHECK(encoder_get_pulses(&encoder, &pulses));
        ESP_ERROR_CHECK(encoder_get_button(&encoder, &button_pressed));

        // Dividing by 4 and holding in range from 0 to 9
        int current_digit = abs(pulses / 4) % 10;

        bool button_just_pressed = (button_pressed == true && last_button_pressed == false);

        // Output the process to the Terminal
        if (pulses != last_pulses) {
            printf("-> Selecting Digit %d:  [%d]     \r", current_digit_index, current_digit);
            fflush(stdout);
        }

        // The FSM of the Safe
        switch (current_state) {
            case STATE_ENTER_DIGIT_1:
                if (button_just_pressed) {
                    entered_code[0] = current_digit;
                    current_digit_index = 2;

                    printf("\n[*] Digit 1 saved: %d. Enter Digit 2:\n", entered_code[0]);

                    // Reseting the digit for the next input
                    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder.pcnt_unit));
                    current_state = STATE_ENTER_DIGIT_2;
                    apply_leds_state(current_state);
                }
                break;

            case STATE_ENTER_DIGIT_2:
                if (button_just_pressed) {
                    entered_code[1] = current_digit;
                    current_digit_index = 3;

                    printf("\n[*] Digit 2 saved: %d. Enter Digit 3:\n", entered_code[1]);

                    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder.pcnt_unit));
                    current_state = STATE_ENTER_DIGIT_3;
                    apply_leds_state(current_state);
                }
                break;

            case STATE_ENTER_DIGIT_3:
                if (button_just_pressed) {
                    entered_code[2] = current_digit;

                    printf("\n[*] Digit 3 saved: %d. Checking password...\n", entered_code[2]);
                    current_state = STATE_CHECK_PASSWORD;
                }
                break;

            case STATE_CHECK_PASSWORD:
                if (entered_code[0] == passcode[0] &&
                    entered_code[1] == passcode[1] &&
                    entered_code[2] == passcode[2]) {
                    printf("\n[ +++ SAFE IS OPENED +++ ]\n");
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_ANGLE_180);
                    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

                    current_state = STATE_UNLOCKED;
                    apply_leds_state(current_state);

                } else {
                    printf("\n[ --- WRONG PASSWORD --- ]\nTry again. Enter Digit 1:\n");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_ANGLE_0);
                    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

                    current_digit_index = 1;
                    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder.pcnt_unit));
                    current_state = STATE_ENTER_DIGIT_1;
                    apply_leds_state(current_state);
                }
                break;

            case STATE_UNLOCKED:
                // Pressing the button while Safe is open will close it
                if (button_just_pressed) {
                    printf("\nClosing safe...\n=== SAFE LOCKED ===\nEnter Digit 1:\n");
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_ANGLE_0);
                    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

                    current_digit_index = 1;
                    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder.pcnt_unit));
                    current_state = STATE_ENTER_DIGIT_1;
                    apply_leds_state(current_state);
                }
                break;
        }

        for (int i = 0; i < LED_COUNT; i++) {
            led_update(&leds[i]);
        }

        last_pulses = pulses;
        last_button_pressed = button_pressed;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void apply_leds_state(safe_state_t state) {
    for (int i = 0; i < LED_COUNT; i++) {
        led_set_mode(&leds[i], LED_MODE_OFF, 0);
    }

    switch (state) {
        case STATE_ENTER_DIGIT_1:
            led_set_mode(&leds[LED_RED], LED_MODE_ON, 0);
            led_set_mode(&leds[LED_YELLOW_1], LED_MODE_BLINK, BLINKING_INTERVAL);
            break;

        case STATE_ENTER_DIGIT_2:
            led_set_mode(&leds[LED_RED], LED_MODE_ON, 0);
            led_set_mode(&leds[LED_YELLOW_1], LED_MODE_ON, 0);
            led_set_mode(&leds[LED_YELLOW_2], LED_MODE_BLINK, BLINKING_INTERVAL);
            break;

        case STATE_ENTER_DIGIT_3:
            led_set_mode(&leds[LED_RED], LED_MODE_ON, 0);
            led_set_mode(&leds[LED_YELLOW_1], LED_MODE_ON, 0);
            led_set_mode(&leds[LED_YELLOW_2], LED_MODE_ON, 0);
            led_set_mode(&leds[LED_YELLOW_3], LED_MODE_BLINK, BLINKING_INTERVAL);
            break;

        case STATE_CHECK_PASSWORD:
            break;

        case STATE_UNLOCKED:
            led_set_mode(&leds[LED_GREEN], LED_MODE_ON, 0);
            break;
    }
}
