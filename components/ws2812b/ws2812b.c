#include "ws2812b.h"
#include "esp_log.h"
#include "driver/rmt.h"

// CLK DIV 1 is equivilent to a tick period of 12.5 ns.
// These values are calculated from the WS2812B datasheet
#define WS2812B_CLK_DIV 1
#define WS2812B_T0H 32
#define WS2812B_T1H 64
#define WS2812B_T0L 68
#define WS2812B_T1L 36


// This value needs to be >= 4000 (50 us) at CLK_DIV 1 by spec
// Set to 4800 (60 us) for some margin
#define WS2812B_RES 4800

#define WS2812B_RMT_CHANNEL 0

// Define the base items to construct the pulse train.
// Again, taken from the WS2812B datasheet
const rmt_item32_t ws2812b_zero_bit = {{{WS2812B_T0H, 1, WS2812B_T0L, 0}}};
const rmt_item32_t ws2812b_one_bit = {{{WS2812B_T1H, 1, WS2812B_T1L, 0}}};
const rmt_item32_t ws2812b_reset = {{{WS2812B_RES / 2, 0, WS2812B_RES / 2, 0}}};

static void led_rmt_init(gpio_num_t data_pin);
static void u8_to_item(const void* src, rmt_item32_t* dest, size_t src_size, size_t dst_size);

static bool WS2812B_INIT = false;
static unsigned int num_leds = 0;
static rmt_item32_t *item_buffer;

// Initialize the RMT perihperal with the appropriate parameters
static void led_rmt_init(gpio_num_t data_pin) {
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = WS2812B_RMT_CHANNEL;
    config.gpio_num = data_pin;
    config.mem_block_num = 1;
    config.tx_config.loop_en = 0;
    config.tx_config.carrier_en = false;
    config.clk_div = 1;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    //ESP_ERROR_CHECK(rmt_translator_init(config.channel, ws2812b_u8_to_item));
}

// Convert byte(s) of data to the appropriate series of items to send to the leds
static void IRAM_ATTR u8_to_item(const void* src, rmt_item32_t* dest, size_t src_size, size_t wanted_num) {
    assert(dest != NULL && src != NULL);
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t* pdest = dest;

    while(size < src_size && num < wanted_num) {
        for(int i = 0; i < 8; i++) {
            if(*psrc & (0x80 >> i)) {
                *pdest = ws2812b_one_bit;
            } else {
                *pdest = ws2812b_zero_bit;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
}

// Convert color array to items and send. We can do this as simple bytescan since
// the color is in memory represented as GRB (what the leds use).
void esp_ws2812b_writeleds(const esp_ws2812b_cgrb_t *colors, size_t num_colors) {
    uint32_t num_items = num_colors * 24;
    u8_to_item(colors, item_buffer, num_colors * sizeof(esp_ws2812b_cgrb_t), num_items);
    item_buffer[num_items + 1] = ws2812b_reset;
    rmt_write_items(WS2812B_RMT_CHANNEL, item_buffer, num_items + 1, true);
}

// Initalize the WS2812B driver, allocate the amount of memory needed.
void esp_ws2812b_init(const esp_ws2812b_config_t *config) {
    num_leds = config->num_leds;
    item_buffer = calloc((num_leds * 24) + 1, sizeof(rmt_item32_t)); // The + 1 is for the RST pulse.
    assert(item_buffer != NULL);
    led_rmt_init(config->data_pin);
    WS2812B_INIT = true;
}

// De-Init and free memory.
void esp_ws2812b_deinit() {
    free(item_buffer);
    ESP_ERROR_CHECK(rmt_driver_uninstall(WS2812B_RMT_CHANNEL));
    WS2812B_INIT = false;
}