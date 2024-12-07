#pragma once

#include <stdint.h>         // general
#include <driver/gpio.h>    // hardware
#include <esp_timer.h>

#include "error.h"

// helper #defines
#define GPIO_HIGH(pin)  gpio_set_level(pin, 1);
#define GPIO_LOW(pin)   gpio_set_level(pin, 0);

// prototypes
int block_until_posedge(gpio_num_t pin, int timeout);
int block_until_negedge(gpio_num_t pin, int timeout);
