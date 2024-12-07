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
