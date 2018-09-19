#ifndef WS2812B_H
#define WS2812B_H
#include "driver/gpio.h"

typedef struct esp_ws2812b_config {
    gpio_num_t data_pin;
    unsigned int num_leds;
} esp_ws2812b_config_t;


// Define an RGB color struct for manipluation,
// using a union force it to be exactly 3 bytes,
// but by struct ordering force the GRB ordering.
typedef union esp_ws2812b_cgrb {
    struct {
        uint8_t g;
        uint8_t r;
        uint8_t b;
    };
    uint8_t raw[3];
} esp_ws2812b_cgrb_t;

void esp_ws2812b_init(const esp_ws2812b_config_t *config);
void esp_ws2812b_writeleds(const esp_ws2812b_cgrb_t* colors, size_t num_colors);
void esp_ws2812b_deinit();

#endif