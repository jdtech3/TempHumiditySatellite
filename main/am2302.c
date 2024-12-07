#include "am2302.h"

static const char* TAG = "am2302";    // logging

void am2302_init(gpio_num_t pin) {
    const gpio_config_t am2302_pin_config = {
        .pin_bit_mask = 1 << pin,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&am2302_pin_config);
    GPIO_HIGH(pin);     // default pullup

    // first reading always fails
    am2302_reading_t dummy;
    am2302_read(&dummy, pin);
}

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
