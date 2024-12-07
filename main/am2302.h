#pragma once

#include <unistd.h>             // general
#include <freertos/FreeRTOS.h>  // RTOS
#include <freertos/task.h>
#include <esp_log.h>            // logging
#include <driver/gpio.h>        // hardware
#include <esp_timer.h>

#include "util.h"
#include "error.h"

// magic
#define AM2302_TIMEOUT      150     // timeout before giving up reading data bit (microseconds)

// errors
#define AM2302_ENACK        1
#define AM2302_ETIMEOUT     2
#define AM2302_ECHKSUM      3

// data strctures
typedef struct am2302_reading {
    float humidity;
    float temperature;
} am2302_reading_t;

// prototypes
void am2302_init(gpio_num_t pin);
int am2302_read(am2302_reading_t* reading, gpio_num_t pin);
