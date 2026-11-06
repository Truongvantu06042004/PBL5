/*
 * Minimal WS2812 driver for ESP-IDF 5.x.
 * The RMT peripheral generates the timing-sensitive GRB data stream, while
 * the rest of the application can keep using RainMaker's HSV light controls.
 */
#include <stdlib.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"

#include "ws2812_driver.h"

#define WS2812_RMT_RESOLUTION_HZ 10000000U

static const char *TAG = "ws2812";

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 3,  /* 0.3 us at 10 MHz */
    .level1 = 0,
    .duration1 = 9,  /* 0.9 us at 10 MHz */
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 9,  /* 0.9 us at 10 MHz */
    .level1 = 0,
    .duration1 = 3,  /* 0.3 us at 10 MHz */
};

static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 0,
    .duration0 = 250, /* 25 us */
    .level1 = 0,
    .duration1 = 250, /* 25 us: reset time = 50 us */
};

static rmt_channel_handle_t s_channel;
static rmt_encoder_handle_t s_encoder;
static uint8_t *s_pixels;
static uint16_t s_led_count;
static bool s_power = true;
static uint16_t s_hue = 180;
static uint8_t s_saturation = 100;
static uint8_t s_brightness = 25;

static size_t ws2812_encoder_callback(const void *data, size_t data_size,
                                      size_t symbols_written, size_t symbols_free,
                                      rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    (void)arg;
    if (symbols_free < 8) {
        return 0;
    }

    const size_t byte_pos = symbols_written / 8;
    const uint8_t *bytes = data;
    if (byte_pos < data_size) {
        for (size_t bit = 0, mask = 0x80; bit < 8; bit++, mask >>= 1) {
            symbols[bit] = (bytes[byte_pos] & mask) ? ws2812_one : ws2812_zero;
        }
        return 8;
    }

    symbols[0] = ws2812_reset;
    *done = true;
    return 1;
}

static void hsv_to_rgb(uint16_t hue, uint8_t saturation, uint8_t value,
                       uint8_t *red, uint8_t *green, uint8_t *blue)
{
    hue %= 360;
    const uint16_t sector = hue / 60;
    const uint16_t fraction = (100 * hue / 60) - (100 * sector);
    const uint16_t p = value * (100 - saturation) / 100;
    const uint16_t q = value * (10000 - fraction * saturation) / 10000;
    const uint16_t t = value * (10000 - saturation * (100 - fraction)) / 10000;
    uint16_t r = 0, g = 0, b = 0;

    switch (sector) {
    case 0: r = value; g = t;     b = p;     break;
    case 1: r = q;     g = value; b = p;     break;
    case 2: r = p;     g = value; b = t;     break;
    case 3: r = p;     g = q;     b = value; break;
    case 4: r = t;     g = p;     b = value; break;
    default:r = value; g = p;     b = q;     break;
    }

    *red = (uint8_t)(r * 255 / 100);
    *green = (uint8_t)(g * 255 / 100);
    *blue = (uint8_t)(b * 255 / 100);
}

static esp_err_t ws2812_show(void)
{
    if (!s_channel || !s_encoder || !s_pixels) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t red = 0, green = 0, blue = 0;
    if (s_power) {
        hsv_to_rgb(s_hue, s_saturation, s_brightness, &red, &green, &blue);
    }

    /* WS2812 expects colour bytes in GRB order. */
    for (uint16_t i = 0; i < s_led_count; i++) {
        s_pixels[i * 3] = green;
        s_pixels[i * 3 + 1] = red;
        s_pixels[i * 3 + 2] = blue;
    }

    const rmt_transmit_config_t tx_config = { .loop_count = 0 };
    ESP_RETURN_ON_ERROR(rmt_enable(s_channel), TAG, "Unable to enable RMT");
    esp_err_t err = rmt_transmit(s_channel, s_encoder, s_pixels,
                                 s_led_count * 3, &tx_config);
    if (err == ESP_OK) {
        err = rmt_tx_wait_all_done(s_channel, -1);
    }
    esp_err_t disable_err = rmt_disable(s_channel);
    return err != ESP_OK ? err : disable_err;
}

esp_err_t ws2812_driver_init(gpio_num_t data_gpio, uint16_t led_count)
{
    if (led_count == 0 || s_channel) {
        return ESP_ERR_INVALID_STATE;
    }

    s_pixels = calloc(led_count, 3);
    if (!s_pixels) {
        return ESP_ERR_NO_MEM;
    }
    s_led_count = led_count;

    const rmt_tx_channel_config_t channel_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = data_gpio,
        .mem_block_symbols = 64,
        .resolution_hz = WS2812_RMT_RESOLUTION_HZ,
        .trans_queue_depth = 1,
    };
    esp_err_t err = rmt_new_tx_channel(&channel_config, &s_channel);
    if (err != ESP_OK) {
        free(s_pixels);
        s_pixels = NULL;
        return err;
    }

    const rmt_simple_encoder_config_t encoder_config = {
        .callback = ws2812_encoder_callback,
        .arg = NULL,
    };
    err = rmt_new_simple_encoder(&encoder_config, &s_encoder);
    if (err != ESP_OK) {
        rmt_del_channel(s_channel);
        s_channel = NULL;
        free(s_pixels);
        s_pixels = NULL;
        return err;
    }

    ESP_LOGI(TAG, "WS2812 ready: GPIO %d, %u LEDs", data_gpio, led_count);
    return ESP_OK;
}

esp_err_t ws2812_driver_set_power(bool power)
{
    s_power = power;
    return ws2812_show();
}

esp_err_t ws2812_driver_set_hsv(uint16_t hue, uint8_t saturation, uint8_t brightness)
{
    if (hue > 360 || saturation > 100 || brightness > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    s_hue = hue;
    s_saturation = saturation;
    s_brightness = brightness;
    return ws2812_show();
}

esp_err_t ws2812_driver_set_hue(uint16_t hue)
{
    return ws2812_driver_set_hsv(hue, s_saturation, s_brightness);
}

esp_err_t ws2812_driver_set_saturation(uint8_t saturation)
{
    return ws2812_driver_set_hsv(s_hue, saturation, s_brightness);
}

esp_err_t ws2812_driver_set_brightness(uint8_t brightness)
{
    return ws2812_driver_set_hsv(s_hue, s_saturation, brightness);
}
