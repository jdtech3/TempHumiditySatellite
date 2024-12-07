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
#include "wifi.h"
#include "mqtt.h"
#include "am2302.h"

static const char* TAG = "main";    // logging
// #define ENABLE_DEBUG_LOGS

// Reading publish helper
void publish_reading(am2302_reading_t* reading) {
    char buf[64];
    snprintf(buf, 64, "{\"temperature\": %.1f,\"humidity\": %.1f}", reading->temperature, reading->humidity);

    ESP_LOGD(TAG, "Publishing %s to MQTT", buf);

    mqtt5_publish(CONFIG_MQTT_TOPIC, buf);
}

void init() {
    // logging
    esp_log_level_set("*", ESP_LOG_WARN);
    #ifdef ENABLE_DEBUG_LOGS
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
        esp_log_level_set("wifi_wrap", ESP_LOG_DEBUG);
        esp_log_level_set("am2302", ESP_LOG_DEBUG);
        esp_log_level_set("mqtt", ESP_LOG_DEBUG);
    #else
        esp_log_level_set(TAG, ESP_LOG_INFO);
        esp_log_level_set("wifi_wrap", ESP_LOG_WARN);
        esp_log_level_set("am2302", ESP_LOG_WARN);
        esp_log_level_set("mqtt", ESP_LOG_WARN);
    #endif

    ESP_LOGW(TAG, "TempHumiditySatellite starting...");
    ESP_LOGW(TAG, "    Time between readings: %.0f s", CONFIG_AM2302_READING_PERIOD / 1000.0f);
    ESP_LOGW(TAG, "    WiFi SSID: %s", CONFIG_ESP_WIFI_SSID);

    ESP_LOGI(TAG, "");  // spacer

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, " > NVS flash init OK");

    // Init WiFi
    wifi_init();
    ESP_LOGI(TAG, " > WiFi init OK");

    // Init TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, " > NETIF (TCP/IP stack) init OK");

    // Init MQTT
    mqtt5_init();
    ESP_LOGI(TAG, " > MQTT5 init OK");

    // Init sensors
    am2302_init(CONFIG_AM2302_PIN);
    generic_output_gpio_init(CONFIG_LED_PIN);
    ESP_LOGI(TAG, " > GPIO+Sensor init OK");

    // Flash LED to indicate init done
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
            publish_reading(&reading);
            ESP_LOGI(TAG, "%.1f°C, %.1f%%RH", reading.temperature, reading.humidity);
    }

    vTaskDelay(CONFIG_AM2302_READING_PERIOD / portTICK_PERIOD_MS);     // delay for min amount (tick period)
}

void app_main(void) {
    init();
 
    while (true) loop();
}
