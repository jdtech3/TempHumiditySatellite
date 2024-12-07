#include "util.h"

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

void generic_output_gpio_init(gpio_num_t pin) {
    const gpio_config_t pin_config = {
        .pin_bit_mask = 1 << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pin_config);
    GPIO_LOW(pin);      // default off
}

