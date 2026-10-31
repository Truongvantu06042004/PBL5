/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "iot_button.h"
#include "ws2812_driver.h"

#include <esp_rmaker_utils.h>

#include DEVELOPMENT_BOARD
#include "app_priv.h"

#define TAG "app_driver"

#define REBOOT_DELAY        2

static bool g_output_state = true;

static void push_btn_cb(void *arg)
{
    app_driver_set_state(!g_output_state);
}

static void factory_reset_trigger(void *arg)
{
    esp_rmaker_factory_reset(0, REBOOT_DELAY);
}

void app_driver_init()
{
    /* Configure push button */
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num     = LIGHT_BUTTON_GPIO,
            .active_level = LIGHT_BUTTON_ACTIVE_LEVEL,
        },
    };
    button_handle_t btn_handle = iot_button_create(&btn_cfg);
    if (btn_handle) {
        /* Register a callback for a button short press event */
        iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, push_btn_cb);
        /* Register a callback for a button long press event */
        iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_START, factory_reset_trigger);
    }

    /* WS2812 uses one RMT data pin. The strip has eight LEDs on GPIO8. */
    ESP_ERROR_CHECK(ws2812_driver_init(WS2812_DATA_GPIO, WS2812_LED_COUNT));
    ESP_ERROR_CHECK(ws2812_driver_set_hsv(DEFAULT_HUE, DEFAULT_SATURATION,
                                          DEFAULT_BRIGHTNESS));
    ESP_ERROR_CHECK(app_light_set_power(DEFAULT_POWER));
}

int IRAM_ATTR app_driver_set_state(bool state)
{
    if (g_output_state != state) {
        g_output_state = state;
        if (g_output_state) {
            // light on
            ESP_LOGI(TAG, "Light ON");
            ws2812_driver_set_power(true);
        } else {
            // light off
            ESP_LOGI(TAG, "Light OFF");
            ws2812_driver_set_power(false);
        }
    }
    return ESP_OK;
}

bool app_driver_get_state(void)
{
    return g_output_state;
}

esp_err_t app_light_set_power(bool power)
{
    if (power) {
        // light on
        ws2812_driver_set_power(true);
    } else {
        // light off
        ws2812_driver_set_power(false);
    }
    return ESP_OK;
}

esp_err_t app_light_set(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    return ws2812_driver_set_hsv(hue, saturation, brightness);
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    return ws2812_driver_set_brightness(brightness);
}

esp_err_t app_light_set_hue(uint16_t hue)
{
    return ws2812_driver_set_hue(hue);
}

esp_err_t app_light_set_saturation(uint16_t saturation)
{
    return ws2812_driver_set_saturation(saturation);
}
