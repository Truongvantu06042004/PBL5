/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// #include <stdio.h>
// #include "esp_log.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "freertos/task.h"

// #include "iot_button.h"
// #include "light_driver.h"

// #include DEVELOPMENT_BOARD
// #include "app_priv.h"

// #define TAG "app_driver"

// static bool g_output_state = true;

// static void push_btn_cb(void *arg)
// {
//     app_driver_set_state(!g_output_state);
// }

// void app_driver_init()
// {
//     /* Configure push button */
//     button_config_t btn_cfg = {
//         .type = BUTTON_TYPE_GPIO,
//         .gpio_button_config = {
//             .gpio_num     = LIGHT_BUTTON_GPIO,
//             .active_level = LIGHT_BUTTON_ACTIVE_LEVEL,
//         },
//     };
//     button_handle_t btn_handle = iot_button_create(&btn_cfg);
//     if (btn_handle) {
//         iot_button_register_cb(btn_handle, BUTTON_PRESS_UP, push_btn_cb);
//     }

//     /**
//      * @brief Light driver initialization
//      */
//     light_driver_config_t driver_config = {
//         .gpio_red        = LIGHT_GPIO_RED,
//         .gpio_green      = LIGHT_GPIO_GREEN,
//         .gpio_blue       = LIGHT_GPIO_BLUE,
//         .gpio_cold       = LIGHT_GPIO_COLD,
//         .gpio_warm       = LIGHT_GPIO_WARM,
//         .fade_period_ms  = LIGHT_FADE_PERIOD_MS,
//         .blink_period_ms = LIGHT_BLINK_PERIOD_MS,
//         .freq_hz         = LIGHT_FREQ_HZ,
//         .clk_cfg         = LEDC_USE_APB_CLK,
//         .duty_resolution = LEDC_TIMER_11_BIT,
//     };
//     ESP_ERROR_CHECK(light_driver_init(&driver_config));
//     light_driver_set_switch(true);
// }

// int IRAM_ATTR app_driver_set_state(bool state)
// {
//     if (g_output_state != state) {
//         g_output_state = state;
//         if (g_output_state) {
//             // light on
//             ESP_LOGI(TAG, "Light ON");
//             light_driver_set_switch(true);
//         } else {
//             // light off
//             ESP_LOGI(TAG, "Light OFF");
//             light_driver_set_switch(false);
//         }
//     }
//     return ESP_OK;
// }

// bool app_driver_get_state(void)
// {
//     return g_output_state;
// }
#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "iot_button.h"
#include "light_driver.h"

#include DEVELOPMENT_BOARD
#include "app_priv.h"


#define TAG "app_driver"


/* =========================
 * Dimming configuration
 * ========================= */

#define DIMMING_STEP       1
#define DIMMING_DELAY_MS   30


/* =========================
 * Global state
 * ========================= */

static bool g_output_state = false;

static int g_brightness = 0;

static bool g_dimming_up = true;



/* =========================
 * Set RGB brightness
 * RGB equal value
 * => white light
 * ========================= */

static void set_rgb_brightness(int brightness)
{
    light_driver_set_rgb(
        brightness,
        brightness,
        brightness
    );
}



/* =========================
 * Dimming task
 * ========================= */

static void dimming_task(void *arg)
{
    while (1)
    {

        if (g_output_state)
        {

            if (g_dimming_up)
            {

                g_brightness += DIMMING_STEP;


                if (g_brightness >= 255)
                {
                    g_brightness = 255;
                    g_dimming_up = false;
                }

            }
            else
            {

                g_brightness -= DIMMING_STEP;


                if (g_brightness <= 0)
                {
                    g_brightness = 0;
                    g_dimming_up = true;
                }

            }


            set_rgb_brightness(g_brightness);

        }


        vTaskDelay(pdMS_TO_TICKS(DIMMING_DELAY_MS));

    }
}



/* =========================
 * Button callback
 * ========================= */

static void push_btn_cb(void *arg)
{
    app_driver_set_state(!g_output_state);
}



/* =========================
 * Driver initialization
 * ========================= */

void app_driver_init(void)
{

    /*
     * Button init
     */

    button_config_t btn_cfg =
    {
        .type = BUTTON_TYPE_GPIO,

        .gpio_button_config =
        {
            .gpio_num = LIGHT_BUTTON_GPIO,

            .active_level = LIGHT_BUTTON_ACTIVE_LEVEL,
        },
    };


    button_handle_t btn_handle = iot_button_create(&btn_cfg);


    if (btn_handle)
    {

        iot_button_register_cb(
            btn_handle,
            BUTTON_SINGLE_CLICK,
            push_btn_cb
        );

    }



    /*
     * LED driver init
     */

    light_driver_config_t driver_config =
    {

        .gpio_red   = LIGHT_GPIO_RED,

        .gpio_green = LIGHT_GPIO_GREEN,

        .gpio_blue  = LIGHT_GPIO_BLUE,

        .gpio_cold  = LIGHT_GPIO_COLD,

        .gpio_warm  = LIGHT_GPIO_WARM,


        .fade_period_ms  = LIGHT_FADE_PERIOD_MS,

        .blink_period_ms = LIGHT_BLINK_PERIOD_MS,

        .freq_hz = LIGHT_FREQ_HZ,


        .clk_cfg = LEDC_USE_APB_CLK,

        .duty_resolution = LEDC_TIMER_11_BIT,

    };


    ESP_ERROR_CHECK(
        light_driver_init(&driver_config)
    );



    /*
     * Initial state OFF
     */

    g_output_state = false;

    g_brightness = 0;

    g_dimming_up = true;


    set_rgb_brightness(0);



    /*
     * Start dimming task
     */

    xTaskCreate(
        dimming_task,
        "dimming_task",
        2048,
        NULL,
        5,
        NULL
    );

}



/* =========================
 * ON / OFF control
 * Called by TCP
 * ========================= */

int app_driver_set_state(bool state)
{

    g_output_state = state;


    if (g_output_state)
    {

        ESP_LOGI(TAG, "LED ON + DIMMING START");


        g_brightness = 0;

        g_dimming_up = true;


        set_rgb_brightness(0);

    }

    else
    {

        ESP_LOGI(TAG, "LED OFF");


        g_brightness = 0;

        g_dimming_up = true;


        set_rgb_brightness(0);

    }


    return ESP_OK;

}



/* =========================
 * Get state
 * ========================= */

bool app_driver_get_state(void)
{

    return g_output_state;

}