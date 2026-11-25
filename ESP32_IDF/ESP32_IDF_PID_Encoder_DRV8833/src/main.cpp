#include <math.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv8833.h"
#include "encoder.h"
#include "pid.h"
#include <etl/atomic.h>
#include <etl/circular_buffer.h>
#include <etl/algorithm.h>

using namespace etl;

// Драйвер DRV8833.
#define MOTOR_IN1 GPIO_NUM_16
#define MOTOR_IN2 GPIO_NUM_17

// Канали KY-040 та калібрування енкодера.
#define ENCODER_A GPIO_NUM_7
#define ENCODER_B GPIO_NUM_15
#define ENCODER_COUNTS_PER_REVOLUTION 80
#define ENCODER_DIRECTION 1
#define ENCODER_GLITCH_FILTER_NS 10000

// Канал АЦП для потенціометра (GPIO4)
#define POT_ADC_CHANNEL ADC_CHANNEL_3

// Налаштування PWM, точність позиціювання та інтервали керування.
#define PWM_FREQUENCY_HZ 2000
#define PWM_RESOLUTION LEDC_TIMER_12_BIT
#define ANGLE_TOLERANCE_DEG 5
#define MIN_MOVING_PWM_PERCENT 50
#define CONTROL_PERIOD_MS 5
#define LOG_PERIOD_MS 300

// Додатний вихід PID задає напрямок IN1, від'ємний — напрямок IN2.
// Коефіцієнти PID; обмеження виходу задаються у відсотках PWM.
static const pid_config_t PID_CONFIG = {
    .kp = 5.0f,
    .ki = 7.0f,
    .kd = 30.0f,
    .output_min = -100.0f,
    .output_max = 100.0f,
    .integral_min = -500.0f,
    .integral_max = 500.0f,
};

// Ненульову команду PID масштабує до робочого діапазону PWM,
// зберігаючи знак, який визначає напрямок обертання.
static float map_motor_pwm(float output_percent) {
    if (output_percent == 0.0f) {
        return 0.0f;
    }

    const float magnitude = MIN_MOVING_PWM_PERCENT +
        fabsf(output_percent) * (100.0f - MIN_MOVING_PWM_PERCENT) / 100.0f;
    return copysignf(magnitude, output_percent);
}

// Функція для мапінгу діапазонів
template <typename T>
T map_range(T x, T in_min, T in_max, T out_min, T out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

struct motor_control_state_t {
    drv8833_t *motor;
    encoder_ctx_t *encoder;
    pid_controller_t *pid;
    adc_oneshot_unit_handle_t adc_handle;
    circular_buffer<int, 8> *adc_filter;
    int32_t initial_count;
    //int64_t setpoint_time_us;
    //size_t setpoint_index;
    float setpoint_deg;
};

// Атомарні значення для обміну даними між callback таймера та циклом логування.
static atomic<int32_t> log_encoder_count{0};
static atomic<int32_t> log_setpoint_deg{0};
static atomic<int8_t> log_output_percent{0};

// Періодично зчитує енкодер, оновлює уставку, виконує PID і керує двигуном.
static void control_timer_callback(void *arg) {
    motor_control_state_t *state = static_cast<motor_control_state_t *>(arg);

    // Читає енкодер
    int32_t encoder_count = 0;
    ESP_ERROR_CHECK(encoder_read(state->encoder, &encoder_count));

    // Перетворює зміну відліків від стартового положення на кут у градусах.
    const float angle_deg =
        ((encoder_count - state->initial_count) * 360.0f /
         ENCODER_COUNTS_PER_REVOLUTION) *
        ENCODER_DIRECTION;

    // Читає АЦП та додаємо у кільцевий буфер
    int raw_adc = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(state->adc_handle, POT_ADC_CHANNEL, &raw_adc));
    state->adc_filter->push(raw_adc);

    // Рахує ковзне середнє
    int adc_sum = 0;
    for (int val : *(state->adc_filter)) {
        adc_sum += val;
    }
    int avg_adc = adc_sum / state->adc_filter->size();

    // Робить мапінг АЦП у градуси та обмеження
    float pot_angle = map_range<float>(static_cast<float>(avg_adc), 0.0f, 4095.0f, 0.0f, 360.0f);
    pot_angle = clamp(pot_angle, 0.0f, 360.0f);

    // Застосовує мертву зону до зміни цілі
    if (fabsf(pot_angle - state->setpoint_deg) > ANGLE_TOLERANCE_DEG) {
        state->setpoint_deg = pot_angle;
        ESP_ERROR_CHECK(pid_reset(state->pid));
    }

    // Обчислює PID
    int8_t pid_output_percent = 0;
    ESP_ERROR_CHECK(pid_compute(state->pid,
                                state->setpoint_deg,
                                angle_deg,
                                CONTROL_PERIOD_MS,
                                &pid_output_percent));

    // У зоні допуску зупиняє двигун і прибирає накопичений інтеграл.
    if (fabsf(state->setpoint_deg - angle_deg) <= ANGLE_TOLERANCE_DEG) {
        pid_output_percent = 0;
        ESP_ERROR_CHECK(pid_reset(state->pid));
    }

    const float output_percent = map_motor_pwm(pid_output_percent);
    ESP_ERROR_CHECK(drv8833_set_speed(
        state->motor, static_cast<int16_t>(lroundf(output_percent))));

    // Зберігає останні дані для виведення в основному циклі.
    log_encoder_count.store(encoder_count, memory_order_relaxed);
    log_setpoint_deg.store(static_cast<int32_t>(state->setpoint_deg), memory_order_relaxed);
    log_output_percent.store(
        static_cast<int8_t>(lroundf(output_percent)),
        memory_order_relaxed);
}

extern "C" void app_main() {
    // Ініціалізація двигуна
    drv8833_t motor = {};
    // Обидва PWM-канали використовують один таймер, частоту й роздільність.
    const drv8833_config_t motor_config = {
        .in1 = {
            .gpio = MOTOR_IN1,
            .channel = LEDC_CHANNEL_0,
            .timer = LEDC_TIMER_0,
            .frequency_hz = PWM_FREQUENCY_HZ,
            .resolution = PWM_RESOLUTION,
            .duty = 0,
            .inverted = false,
        },
        .in2 = {
            .gpio = MOTOR_IN2,
            .channel = LEDC_CHANNEL_1,
            .timer = LEDC_TIMER_0,
            .frequency_hz = PWM_FREQUENCY_HZ,
            .resolution = PWM_RESOLUTION,
            .duty = 0,
            .inverted = false,
        },
    };
    ESP_ERROR_CHECK(drv8833_init(&motor, &motor_config));

    // Ініціалізація Енкодера
    encoder_ctx_t encoder = {};
    ESP_ERROR_CHECK(encoder_init(&encoder,
                                 ENCODER_A,
                                 ENCODER_B,
                                 GPIO_NUM_NC,
                                 ENCODER_GLITCH_FILTER_NS));

    // Ініціалізація PID
    pid_controller_t angle_pid = {};
    ESP_ERROR_CHECK(pid_init(&angle_pid, &PID_CONFIG));

    // Ініціалізація АЦП
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, POT_ADC_CHANNEL, &config));

    // Буфер фільтрації
    static circular_buffer<int, 8> adc_filter;

    // Поточний відлік енкодера задає нульову позицію для кутів у програмі.
    int32_t initial_count = 0;
    ESP_ERROR_CHECK(encoder_read(&encoder, &initial_count));

    printf("Starting PID control via Potentiometer. encoder: %d counts/rev.\n",
           ENCODER_COUNTS_PER_REVOLUTION);

    //const int64_t start_time_us = esp_timer_get_time();
    motor_control_state_t control_state = {
        .motor = &motor,
        .encoder = &encoder,
        .pid = &angle_pid,
        .adc_handle = adc1_handle,
        .adc_filter = &adc_filter,
        .initial_count = initial_count,
        //.setpoint_time_us = start_time_us,
        .setpoint_deg = 0.0f,
        //.setpoint_deg = ANGLE_SETPOINTS_DEG[0],
    };

    // Таймер запускає цикл керування окремо від повільнішого логування.
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = control_timer_callback;
    timer_args.arg = &control_state;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "motor_control";
    esp_timer_handle_t control_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &control_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(
        control_timer, static_cast<uint64_t>(CONTROL_PERIOD_MS) * 1000ULL));

    float previous_logged_setpoint = -1.0f;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(LOG_PERIOD_MS));

        // Виводить останні атомарно збережені показання, не затримуючи PID.
        const int32_t encoder_count =
            log_encoder_count.load(memory_order_relaxed);
        const float setpoint_deg =
            static_cast<float>(log_setpoint_deg.load(memory_order_relaxed));
        const int8_t output_percent =
            log_output_percent.load(memory_order_relaxed);

        const float angle_deg =
            ((static_cast<int64_t>(encoder_count) - initial_count) * 360.0f /
             ENCODER_COUNTS_PER_REVOLUTION) *
            ENCODER_DIRECTION;

        if (fabsf(setpoint_deg - previous_logged_setpoint) > 0.1f) {
            printf("New angle target from POT: %.1f deg.\n", setpoint_deg);
            previous_logged_setpoint = setpoint_deg;
        }

        printf("Angle: %.1f deg, target: %.1f deg, PWM: %.0f%%\n",
               angle_deg,
               setpoint_deg,
               static_cast<float>(output_percent));
    }
}
