#include <stdbool.h>
#include <stdint.h>
#include "esp_log.h"
#include "iot_button.h"
#include "ws2812_driver.h"
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_utils.h"
#include DEVELOPMENT_BOARD
#include "app_priv.h"

#define TAG "app_driver"
static bool g_output_state = DEFAULT_POWER;
extern esp_rmaker_device_t *light_device;

static void push_btn_cb(void *arg)
{
    bool next = !g_output_state;
    if (app_light_set_power(next) == ESP_OK && light_device) {
        esp_rmaker_param_t *power = esp_rmaker_device_get_param_by_name(light_device, ESP_RMAKER_DEF_POWER_NAME);
        if (power) esp_rmaker_param_update_and_report(power, esp_rmaker_bool(next));
    }
}

static void factory_reset_trigger(void *arg)
{
    esp_rmaker_factory_reset(0, 2);
}

void app_driver_init(void)
{
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = { .gpio_num = LIGHT_BUTTON_GPIO, .active_level = LIGHT_BUTTON_ACTIVE_LEVEL },
    };
    button_handle_t btn = iot_button_create(&btn_cfg);
    if (btn) {
        iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, push_btn_cb);
        iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, factory_reset_trigger);
    }
    ESP_ERROR_CHECK(ws2812_driver_init(WS2812_DATA_GPIO, WS2812_LED_COUNT));
    ESP_ERROR_CHECK(ws2812_driver_set_hsv(DEFAULT_HUE, DEFAULT_SATURATION, DEFAULT_BRIGHTNESS));
    ESP_ERROR_CHECK(app_light_set_power(DEFAULT_POWER));
}

int app_driver_set_state(bool state) { return app_light_set_power(state); }
bool app_driver_get_state(void) { return g_output_state; }
esp_err_t app_light_set_power(bool power)
{
    esp_err_t err = ws2812_driver_set_power(power);
    if (err == ESP_OK) g_output_state = power;
    return err;
}
esp_err_t app_light_set(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    return ws2812_driver_set_hsv(hue, saturation, brightness);
}
esp_err_t app_light_set_brightness(uint16_t brightness) { return ws2812_driver_set_brightness(brightness); }
esp_err_t app_light_set_hue(uint16_t hue) { return ws2812_driver_set_hue(hue); }
esp_err_t app_light_set_saturation(uint16_t saturation) { return ws2812_driver_set_saturation(saturation); }
