
#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"

#include "iot_led.h"


static const char *TAG = "iot_light";


#define MAX_PWM_DUTY 8191



typedef struct
{
    bool running;

} breath_channel_t;



typedef struct
{

    ledc_mode_t speed_mode;

    ledc_timer_t timer_num;

    breath_channel_t breath[LEDC_CHANNEL_MAX];


} iot_light_t;



static iot_light_t *g_light_config = NULL;

static TaskHandle_t breath_task_handle = NULL;



static uint32_t convert_duty(uint8_t value)
{
    return ((uint32_t)value * MAX_PWM_DUTY) / 255;
}





static void breath_task(void *arg)
{

    while(1)
    {

        // tăng sáng

        for(int brightness = 0; brightness <= 255; brightness++)
        {

            for(int ch = 0; ch < LEDC_CHANNEL_MAX; ch++)
            {

                if(g_light_config->breath[ch].running)
                {

                    ledc_set_duty(
                        g_light_config->speed_mode,
                        ch,
                        convert_duty(brightness)
                    );


                    ledc_update_duty(
                        g_light_config->speed_mode,
                        ch
                    );

                }

            }


            vTaskDelay(pdMS_TO_TICKS(10));

        }



        // giảm sáng

        for(int brightness = 255; brightness >= 0; brightness--)
        {

            for(int ch = 0; ch < LEDC_CHANNEL_MAX; ch++)
            {

                if(g_light_config->breath[ch].running)
                {

                    ledc_set_duty(
                        g_light_config->speed_mode,
                        ch,
                        convert_duty(brightness)
                    );


                    ledc_update_duty(
                        g_light_config->speed_mode,
                        ch
                    );

                }

            }


            vTaskDelay(pdMS_TO_TICKS(10));

        }

    }

}





esp_err_t iot_led_init(
        ledc_timer_t timer_num,
        ledc_mode_t speed_mode,
        uint32_t freq_hz,
        ledc_clk_cfg_t clk_cfg,
        ledc_timer_bit_t duty_resolution)
{


    ledc_timer_config_t timer_cfg =
    {
        .speed_mode = speed_mode,

        .timer_num = timer_num,

        .freq_hz = freq_hz,

        .duty_resolution = duty_resolution,

        .clk_cfg = clk_cfg
    };



    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_cfg)
    );



    g_light_config = calloc(
        1,
        sizeof(iot_light_t)
    );



    if(g_light_config == NULL)
    {
        return ESP_ERR_NO_MEM;
    }



    g_light_config->speed_mode = speed_mode;

    g_light_config->timer_num = timer_num;



    ESP_LOGI(
        TAG,
        "LED driver initialized"
    );


    return ESP_OK;

}





esp_err_t iot_led_deinit()
{

    if(g_light_config)
    {

        free(g_light_config);

        g_light_config = NULL;

    }


    return ESP_OK;

}





esp_err_t iot_led_regist_channel(
        ledc_channel_t channel,
        gpio_num_t gpio_num)
{


    ledc_channel_config_t channel_cfg =
    {

        .gpio_num = gpio_num,

        .channel = channel,

        .speed_mode =
            g_light_config->speed_mode,

        .timer_sel =
            g_light_config->timer_num,

        .duty = 0,

        .hpoint = 0

    };



    return ledc_channel_config(
        &channel_cfg
    );

}





esp_err_t iot_led_set_channel(
        ledc_channel_t channel,
        uint8_t value,
        uint32_t fade_ms)
{


    ledc_set_duty(
        g_light_config->speed_mode,
        channel,
        convert_duty(value)
    );


    ledc_update_duty(
        g_light_config->speed_mode,
        channel
    );


    return ESP_OK;

}





esp_err_t iot_led_start_blink(
        ledc_channel_t channel,
        uint8_t value,
        uint32_t period_ms,
        bool fade_flag)
{


    g_light_config->breath[channel].running = true;



    if(breath_task_handle == NULL)
    {

        xTaskCreate(
            breath_task,
            "breath_task",
            4096,
            NULL,
            5,
            &breath_task_handle
        );

    }


    return ESP_OK;

}





esp_err_t iot_led_stop_blink(
        ledc_channel_t channel)
{

    g_light_config->breath[channel].running = false;


    return ESP_OK;

}





esp_err_t iot_led_set_gamma_table(
        const uint16_t gamma_table[GAMMA_TABLE_SIZE])
{

    return ESP_OK;

}