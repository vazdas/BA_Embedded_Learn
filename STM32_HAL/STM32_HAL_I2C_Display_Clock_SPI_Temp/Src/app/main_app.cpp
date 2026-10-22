#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "i2c/i2c.h"
#include "oled/oled.h"
#include "ds1307/ds1307.h"
#include "bme280_spi/bme280_spi.h"

#define I2C_TIMEOUT_MS 100
#define I2C_SPEED_HZ 100000U

static I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;

// Array for weekdays
static const char *weekdays[] = {
    "Err", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

extern "C" void main_cpp()
{
    HAL_StatusTypeDef status;

    // Initialization of the I2C1 hardware bus
    status = I2C_Init(&hi2c1, I2C1, I2C_SPEED_HZ);
    if (status != HAL_OK)
    {
        printf("[I2C] I2C1 initialization failed (status=%d)\r\n", (int)status);
        Error_Handler();
    }
    printf("[I2C] Initialized successfully\r\n");

    // Initialization of the DS1307 clock
    DS1307_HandleTypeDef *rtc = NULL;
    status = DS1307_Init(&hi2c1, &rtc);
    if (status != HAL_OK)
    {
        printf("[RTC] DS1307 Init failed (status=%d)\r\n", (int)status);
        // If there is no clock, we stop.
        Error_Handler();
    }
    printf("[RTC] DS1307 ready\r\n");

    // Check if the clock is running. If not set the initial time.
    bool rtc_running = false;
    DS1307_IsRunning(rtc, &rtc_running);
    if (!rtc_running)
    {
        printf("[RTC] Clock stopped. Setting default time...\r\n");
        DS1307_DateTimeTypeDef default_time = {
            .year = 2026,
            .month = 2,
            .day = 14,
            .weekday = 7,  // Saturday
            .hour = 15,
            .minute = 33,
            .second = 59
        };
        DS1307_SetDateTime(rtc, &default_time);
    }

    // Initialization of the SSD1306 OLED display
    OLED_SettingsTypeDef oled_settings = OLED_SETTINGS_128X64_DEFAULT();
    OLED_HandleTypeDef *oled = NULL;

    status = OLED_Init(&hi2c1, &oled_settings, &oled);
    if (status != HAL_OK)
    {
        printf("[OLED] Initialization failed (status=%d)\r\n", (int)status);
        Error_Handler();
    }
    printf("[OLED] SSD1306 ready\r\n");

    // Switch the pin to 1
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(10);

    // Chekking Chip ID
    uint8_t chip_id = 0;
    bme280_spi_read(0xD0, &chip_id, 1);

    if (chip_id == 0x60) {
        printf("[BME280] Found! Chip ID: 0x60\r\n");
    } else {
        printf("[BME280] Error! Wrong Chip ID: 0x%02X\r\n", chip_id);
    }

    // Reading the calibration data
    Bme280Calib calib = {};
    bme280_read_calibration(&calib);

    // Setting up BME280 for measuring
    // Register 0xF2 (ctrl_hum) = 0x01 (Humadity x1)
    bme280_spi_write(0xF2, 0x01);

    // Регістр 0xF4 (ctrl_meas) = 0x25 (Темп x1, Тиск x1, Forced mode)
    bme280_spi_write(0xF4, 0x25);

    printf("[BME280] Initialization complete\r\n");

    // Clear the screen
    OLED_Clear(oled);

    // Main update cycle (at least once per second)
    DS1307_DateTimeTypeDef current_time;
    Bme280Compensated current_temp;

    uint32_t last_update_tick = 0;

    bme280_spi_write(0xF2, 0x01);
    bme280_spi_write(0xF4, 0x25);

    while (1)
    {
        uint32_t current_tick = HAL_GetTick();

        if (current_tick - last_update_tick >= 1000)
        {
            last_update_tick = current_tick;

            // Read the current time
            status = DS1307_GetDateTime(rtc, &current_time);
            if (status != HAL_OK) {
                printf("[RTC] Failed to read time!\r\n");
            }

            // Reading data from BME280
            uint8_t raw_data[8] = {0};
            bme280_spi_read(0xF7, raw_data, 8);

            // Combining the data
            int32_t raw_press = ((int32_t)raw_data[0] << 12) | ((int32_t)raw_data[1] << 4) | ((int32_t)raw_data[2] >> 4);
            int32_t raw_temp  = ((int32_t)raw_data[3] << 12) | ((int32_t)raw_data[4] << 4) | ((int32_t)raw_data[5] >> 4);
            int32_t raw_hum   = ((int32_t)raw_data[6] << 8)  | (int32_t)raw_data[7];

            current_temp = bme280_compensate(raw_temp, raw_press, raw_hum, &calib);

            // Next measurements
            bme280_spi_write(0xF2, 0x01);
            bme280_spi_write(0xF4, 0x25);

            // Print to terminal
            printf("Time: %02u:%02u:%02u | T: %.1f C | RH: %.1f %% | P: %.0f hPa\r\n",
                   current_time.hour, current_time.minute, current_time.second,
                   current_temp.temperature_c, current_temp.humidity_rh, current_temp.pressure_hpa);

            // Displaying the time: HH:MM:SS
            OLED_Printf(oled, 1, 2, "%02u:%02u:%02u",
                        current_time.hour,
                        current_time.minute,
                        current_time.second);

            // Displaying the date: EEE DD.MM.YYYY
            uint8_t wd = current_time.weekday;
            if (wd < 1 || wd > 7) wd = 0;

            OLED_Printf(oled, 3, 1, "%s %02u.%02u.%04u",
                        weekdays[wd],
                        current_time.day,
                        current_time.month,
                        current_time.year);

            OLED_Printf(oled, 5, 1, "T:  %.1f C", current_temp.temperature_c);
            OLED_Printf(oled, 6, 1, "RH: %.1f %%", current_temp.humidity_rh);
            OLED_Printf(oled, 7, 1, "P:  %.0f hPa", current_temp.pressure_hpa);
        }
    }
}
