/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


#include <stdio.h>              // general
#include <stdbool.h>
#include <unistd.h>
#include <esp_log.h>            // logging
#include <freertos/FreeRTOS.h>  // RTOS
#include <freertos/task.h>
#include <driver/gpio.h>        // hardware
#include <esp_timer.h>

#include "util.h"
#include "error.h"
#include "am2302.h"

static const char* TAG = "main";    // logging
#define ENABLE_DEBUG_LOGS

// Onboard LED

void led_init(gpio_num_t pin) {
    const gpio_config_t led_pin_config = {
        .pin_bit_mask = 1 << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_pin_config);
    GPIO_LOW(pin);      // default off
}

// Rest

void init() {
    // logging
    esp_log_level_set("*", ESP_LOG_WARN);
    #ifdef ENABLE_DEBUG_LOGS
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
        esp_log_level_set("am2302", ESP_LOG_DEBUG);
    #else
        esp_log_level_set(TAG, ESP_LOG_INFO);
    #endif

    ESP_LOGW(TAG, "TempHumiditySatellite starting...");
    ESP_LOGW(TAG, "    Time between readings: %.0f s", CONFIG_AM2302_READING_PERIOD / 1000.0f);

    am2302_init(CONFIG_AM2302_PIN);
    led_init(CONFIG_LED_PIN);

    GPIO_HIGH(CONFIG_LED_PIN);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    GPIO_LOW(CONFIG_LED_PIN);

    ESP_LOGW(TAG, "Initialization complete ✓");
}

void loop() {
    am2302_reading_t reading;

    GPIO_HIGH(CONFIG_LED_PIN);
    int ret = am2302_read(&reading, CONFIG_AM2302_PIN);
    GPIO_LOW(CONFIG_LED_PIN);

    switch (-ret) {
        case AM2302_ENACK:
            ESP_LOGE(TAG, "AM2302 no ACK received when read requested");
            break;
        case AM2302_ETIMEOUT:
            ESP_LOGE(TAG, "AM2302 timed out when reading data");
            break;
        case AM2302_ECHKSUM:
            ESP_LOGE(TAG, "AM2302 checksum did not match");
            break;
        default:
            ESP_LOGI(TAG, "AM2302 reading: %.1fC, %.1f%%RH", reading.temperature, reading.humidity);
    }

    vTaskDelay(CONFIG_AM2302_READING_PERIOD / portTICK_PERIOD_MS);     // delay for min amount (tick period)
}

void app_main(void) {
    init();
 
    while (true) loop();
}
