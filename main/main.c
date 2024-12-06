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

// Settings

#define AM2302_PIN  23
#define LED_PIN     2

#define AM2302_READING_PERIOD   2000    // time between readings in ms

// Errors

#define ETIMEOUT    1

// Helpers

static const char* TAG = "main";    // logging

#define GPIO_HIGH(pin)  gpio_set_level(pin, 1);
#define GPIO_LOW(pin)   gpio_set_level(pin, 0);

int block_until_posedge(gpio_num_t pin, int timeout) {  // timeout in microseconds
    int64_t start = esp_timer_get_time();

    // wait for negative
    while (gpio_get_level(pin))
        if (esp_timer_get_time()-start > timeout) return -ETIMEOUT;

    // ...then positive
    while (!gpio_get_level(pin))
        if (esp_timer_get_time()-start > timeout) return -ETIMEOUT;

    return 0;
}

int block_until_negedge(gpio_num_t pin, int timeout) {  // timeout in microseconds
    int64_t start = esp_timer_get_time();

    // wait for positive
    while (!gpio_get_level(pin))
        if (esp_timer_get_time()-start > timeout) return -ETIMEOUT;

    // ...then negative
    while (gpio_get_level(pin))
        if (esp_timer_get_time()-start > timeout) return -ETIMEOUT;
    
    return 0;
}

// AM2302 driver

typedef struct am2302_reading {
    float humidity;
    float temperature;
} am2302_reading_t;

void am2302_init(gpio_num_t pin) {
    const gpio_config_t am2302_pin_config = {
        .pin_bit_mask = 1 << pin,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&am2302_pin_config);
    GPIO_HIGH(AM2302_PIN);      // default pullup
}

#define AM2302_TIMEOUT      150

#define AM2302_ENACK        1
#define AM2302_ETIMEOUT     2
#define AM2302_ECHKSUM      3

int am2302_read(am2302_reading_t* reading, gpio_num_t pin) {
    // request read
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    GPIO_LOW(pin);
    usleep(1500);       // 1-10 ms
    GPIO_HIGH(pin);
    usleep(30);         // 20-40 us
    
    // check sensor ack
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    usleep(40);
    uint8_t ack1 = gpio_get_level(pin);
    usleep(80);
    uint8_t ack2 = gpio_get_level(pin);
    if (ack1 != 0 || ack2 != 1) return -AM2302_ENACK;

    // read data
    if (block_until_posedge(pin, AM2302_TIMEOUT) == -ETIMEOUT) return -AM2302_ETIMEOUT;     // wait til data starts to come back

    uint64_t raw_data = 0;
    for (int i = 0; i < 40; i++) {          // 40 bits
        int64_t pulse_start = esp_timer_get_time();
        if (block_until_posedge(pin, AM2302_TIMEOUT) == -ETIMEOUT) return -AM2302_ETIMEOUT;

        raw_data <<= 1;
        raw_data |= (esp_timer_get_time() - pulse_start) > 100;     // start: 50 us + 0: 26-28 us or 1: 70 us
    }

    ESP_LOGD(TAG, "raw data: %llx", raw_data);


    // reset to pullup
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    GPIO_HIGH(pin);

    // verify checksum
    uint16_t sum = (raw_data >> 8 & 0xFF) + (raw_data >> 16 & 0xFF) + (raw_data >> 24 & 0xFF) + (raw_data >> 32 & 0xFF);
    if ((sum & 0xFF) != (raw_data & 0xFF)) return -AM2302_ECHKSUM;

    // convert and fill reading struct
    uint16_t temp_raw = raw_data >> 8 & 0xFFFF;
    uint16_t humidity_raw = raw_data >> 24 & 0xFFFF;
    reading->temperature = ((int16_t)temp_raw) / 10.0;
    reading->humidity = ((int)humidity_raw) / 10.0;

    return 0;
}

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
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    // esp_log_level_set("main", ESP_LOG_INFO);

    ESP_LOGI(TAG, "TempHumiditySatellite starting!");

    am2302_init(AM2302_PIN);
    led_init(LED_PIN);

    GPIO_HIGH(LED_PIN)
    vTaskDelay(500 / portTICK_PERIOD_MS);
    GPIO_LOW(LED_PIN);
}

void loop() {
    am2302_reading_t reading;

    GPIO_HIGH(LED_PIN);
    int ret = am2302_read(&reading, AM2302_PIN);
    GPIO_LOW(LED_PIN);

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

    vTaskDelay(AM2302_READING_PERIOD / portTICK_PERIOD_MS);     // delay for min amount (tick period)
}

void app_main(void) {
    init();
 
    while (true) loop();
}
