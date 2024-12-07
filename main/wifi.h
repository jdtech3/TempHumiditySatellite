// Based on WiFi station example (https://github.com/espressif/esp-idf/blob/v5.3.2/examples/wifi/getting_started/station/)
#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_CONNECT_RETRIES    3

void wifi_init();
