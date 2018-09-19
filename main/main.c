#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "ws2812b.h"
#include "driver/uart.h"

/* --------------------------------------- */
/* ---------- FIRMWARE SETTINGS ---------- */
/* --------------------------------------- */

// Number of LEDS in strip
#define NUM_LEDS 262

// UART Settings. It is configured by default to use the UART for the console
// which makes it easy to hook up using the USB-SERIAL converter
#define APP_UART_TXD  (GPIO_NUM_1)
#define APP_UART_RXD  (GPIO_NUM_3)
#define APP_UART_RTS  (UART_PIN_NO_CHANGE)
#define APP_UART_CTS (UART_PIN_NO_CHANGE)

#define APP_UART_CHANNEL (UART_NUM_0)

/* NEEDED CONSTANTS DONT TOUCH */
#define MAX(a,b) (a > b ? a : b)

#define BUF_SIZE MAX((NUM_LEDS * 3),128)

#define APP_UART_MAX_WAIT (132 / portTICK_RATE_MS)

static esp_ws2812b_cgrb_t leds[NUM_LEDS] = { 0 };

// Defines constants for 
static const uint8_t prefix[] = {'A', 'd', 'a'};

static uint8_t hi, lo, chk;
static int len;

void app_main()
{   

    // Configure the WS2812B driver
    esp_ws2812b_config_t led_config;
    led_config.num_leds = NUM_LEDS;
    led_config.data_pin = 18;
    esp_ws2812b_init(&led_config);


    // Configure the UART and install the driver
    uart_config_t uart_config = {
        .baud_rate = 500000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(APP_UART_CHANNEL, &uart_config);
    uart_set_pin(APP_UART_CHANNEL, APP_UART_TXD, APP_UART_RXD, APP_UART_RTS, APP_UART_CTS);
    uart_driver_install(APP_UART_CHANNEL, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Allocate the recieve buffer
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    // Enter a endless loop to process UART information
    while(1) {
        // This matches the magic prefix above, the system
        // will not process commands until this passes
        int matched = 0;
        while(matched < sizeof(prefix)){
            // Read one byte at a time, looking for the prefix bytes
            len = uart_read_bytes(APP_UART_CHANNEL, data, 1, APP_UART_MAX_WAIT);
            if(len > 0){
                // Since we are always reading one byte into the buffer, we always compare
                // against data[0]
                if(prefix[matched] == data[0]) { 
                    matched++;
                }else {
                    matched = 0;
                }
            }
        }
        // Get the hi, lo, and checksums from the application
        len = uart_read_bytes(APP_UART_CHANNEL, data, 3, APP_UART_MAX_WAIT);

        // if for some reason there was not enough bytes in one frame time, loop
        if(len < 3) {
            continue;
        }

        hi = data[0];
        lo = data[1];
        chk = data[2];

        // Don't Bother with checksums, they reallly don't matter for this application.

        // if (chk != (hi ^ lo ^ 0x55)) {
        //     continue;
        // }

        len = uart_read_bytes(APP_UART_CHANNEL, data, NUM_LEDS * 3, APP_UART_MAX_WAIT);
        // If the bytes are not there, loop
        if (len < NUM_LEDS * 3) {
            continue;
        }

        // Clear color store
        memset(leds, 0, NUM_LEDS * sizeof(esp_ws2812b_cgrb_t));

        // Fill color store from UART
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i].r = data[i*3];
            leds[i].g = data[i*3 + 1];
            leds[i].b = data[i*3 + 2];
        }

        // Write the color store to the LEDS
        esp_ws2812b_writeleds(leds, NUM_LEDS);
    }
}
