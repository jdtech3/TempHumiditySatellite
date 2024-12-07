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

void init() {
    // logging
    esp_log_level_set("*", ESP_LOG_WARN);
    #ifdef ENABLE_DEBUG_LOGS
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
        esp_log_level_set("am2302", ESP_LOG_DEBUG);
    #else
        esp_log_level_set(TAG, ESP_LOG_INFO);
        esp_log_level_set("am2302", ESP_LOG_INFO);
    #endif

    ESP_LOGW(TAG, "TempHumiditySatellite starting...");
    ESP_LOGW(TAG, "    Time between readings: %.0f s", CONFIG_AM2302_READING_PERIOD / 1000.0f);
    ESP_LOGW(TAG, "    WiFi SSID: %s", CONFIG_ESP_WIFI_SSID);

    am2302_init(CONFIG_AM2302_PIN);
    generic_output_gpio_init(CONFIG_LED_PIN);

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
