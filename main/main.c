/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


#include <stdio.h>              // general
#include <stdbool.h>
#include <unistd.h>
#include <freertos/FreeRTOS.h>  // RTOS
#include <freertos/task.h>
#include <driver/gpio.h>        // hardware

// Settings

#define AM2302_PIN  23
#define LED_PIN     2

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
    gpio_set_level(AM2302_PIN, 1);
}

#define AM2302_ENACK   1

int am2302_read(am2302_reading_t* reading, gpio_num_t pin) {
    // TODO: properly check edges instead of relying on timings

    // request read
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    usleep(1500);
    gpio_set_level(pin, 1);
    usleep(30);
    
    // check sensor ack
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    usleep(40);
    uint8_t ack1 = gpio_get_level(pin);
    usleep(80);
    uint8_t ack2 = gpio_get_level(pin);
    if (ack1 != 0 || ack2 != 1) return -AM2302_ENACK;
    usleep(90);

    // read data
    uint64_t raw_data = 0;

    for (int i = 0; i < 40; i++) {          // 40 bits
        usleep((raw_data & 1) ? 43 : 10);   // if last bit is 1, need 43 us, otherwise 10 us
        uint8_t bit_reading1 = gpio_get_level(pin);
        usleep(27);
        uint8_t bit_reading2 = gpio_get_level(pin);
        
        raw_data <<= 1;
        raw_data |= bit_reading1 & bit_reading2;    // if both 1 (long pulse), then 1. otherwise (short pulse) 0
    }

    usleep(1000);

    printf("raw data: %llx\n", raw_data);

    // reset to pullup
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);

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
}

// Rest

void init() {
    printf("TempHumiditySatellite starting!\n");

    am2302_init(AM2302_PIN);
    led_init(LED_PIN);

    gpio_set_level(LED_PIN, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(LED_PIN, 0);
}

void loop() {
    am2302_reading_t reading;

    gpio_set_level(LED_PIN, 1);
    int ret = am2302_read(&reading, AM2302_PIN);
    gpio_set_level(LED_PIN, 0);
    
    printf("reading returned: %d\n", ret);

    vTaskDelay(5000 / portTICK_PERIOD_MS);     // delay for min amount (tick period)
}

void app_main(void) {
    init();
 
    while (true) loop();
}
