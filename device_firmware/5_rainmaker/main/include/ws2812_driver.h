#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/gpio.h"

/** Initialise a WS2812 strip driven through ESP-IDF's RMT peripheral. */
esp_err_t ws2812_driver_init(gpio_num_t data_gpio, uint16_t led_count);

esp_err_t ws2812_driver_set_power(bool power);
esp_err_t ws2812_driver_set_hsv(uint16_t hue, uint8_t saturation, uint8_t brightness);
esp_err_t ws2812_driver_set_hue(uint16_t hue);
esp_err_t ws2812_driver_set_saturation(uint8_t saturation);
esp_err_t ws2812_driver_set_brightness(uint8_t brightness);
