#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "esp_timer.h"

#include "button_adc.h"


static const char *TAG = "adc button";


#define NO_OF_SAMPLES CONFIG_ADC_BUTTON_SAMPLE_TIMES

#define ADC_BUTTON_ADC_UNIT ADC_UNIT_1

#define ADC_BUTTON_MAX_CHANNEL CONFIG_ADC_BUTTON_MAX_CHANNEL

#define ADC_BUTTON_MAX_BUTTON CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL



typedef struct
{
    uint16_t min;
    uint16_t max;

} button_data_t;



typedef struct
{
    adc_channel_t channel;

    uint8_t is_init;

    button_data_t btns[ADC_BUTTON_MAX_BUTTON];

    uint64_t last_time;

} btn_adc_channel_t;



typedef struct
{
    bool is_configured;

    adc_oneshot_unit_handle_t adc_handle;

    adc_cali_handle_t cali_handle;

    btn_adc_channel_t ch[ADC_BUTTON_MAX_CHANNEL];

} adc_button_t;



static adc_button_t g_button = {0};



static int find_unused_channel(void)
{
    for(int i = 0; i < ADC_BUTTON_MAX_CHANNEL; i++)
    {
        if(g_button.ch[i].is_init == 0)
        {
            return i;
        }
    }

    return -1;
}



static int find_channel(adc_channel_t channel)
{
    for(int i = 0; i < ADC_BUTTON_MAX_CHANNEL; i++)
    {
        if(g_button.ch[i].channel == channel)
        {
            return i;
        }
    }

    return -1;
}




esp_err_t button_adc_init(const button_adc_config_t *config)
{

    if(config == NULL)
        return ESP_ERR_INVALID_ARG;



    int ch_index = find_channel(config->adc_channel);



    if(ch_index < 0)
    {

        ch_index = find_unused_channel();


        if(ch_index < 0)
            return ESP_ERR_NO_MEM;



        g_button.ch[ch_index].channel =
            config->adc_channel;


        g_button.ch[ch_index].is_init = 1;



        if(!g_button.is_configured)
        {

            adc_oneshot_unit_init_cfg_t init_config =
            {
                .unit_id = ADC_BUTTON_ADC_UNIT,
            };


            ESP_ERROR_CHECK(
                adc_oneshot_new_unit(
                    &init_config,
                    &g_button.adc_handle
                )
            );



            adc_oneshot_chan_cfg_t chan_cfg =
            {
                .bitwidth = ADC_BITWIDTH_12,
                .atten = ADC_ATTEN_DB_12,
            };



            ESP_ERROR_CHECK(
                adc_oneshot_config_channel(
                    g_button.adc_handle,
                    config->adc_channel,
                    &chan_cfg
                )
            );



            adc_cali_curve_fitting_config_t cali_config =
            {
                .unit_id = ADC_BUTTON_ADC_UNIT,
                .chan = config->adc_channel,
                .atten = ADC_ATTEN_DB_12,
                .bitwidth = ADC_BITWIDTH_12,
            };



            adc_cali_create_scheme_curve_fitting(
                &cali_config,
                &g_button.cali_handle
            );

            // adc_cali_create_scheme_line_fitting(
            //     &cali_config,
            //     &g_button.cali_handle
            // );


            g_button.is_configured = true;

        }

    }



    g_button.ch[ch_index]
    .btns[config->button_index]
    .min = config->min;



    g_button.ch[ch_index]
    .btns[config->button_index]
    .max = config->max;



    ESP_LOGI(TAG, "ADC button initialized");


    return ESP_OK;

}




static uint32_t get_adc_voltage(adc_channel_t channel)
{

    int raw = 0;

    int total = 0;


    for(int i = 0; i < NO_OF_SAMPLES; i++)
    {

        adc_oneshot_read(
            g_button.adc_handle,
            channel,
            &raw
        );


        total += raw;

    }


    raw = total / NO_OF_SAMPLES;



    int voltage = 0;



    adc_cali_raw_to_voltage(
        g_button.cali_handle,
        raw,
        &voltage
    );



    return voltage;

}





uint8_t button_adc_get_key_level(void *button_index)
{

    uint32_t ch =
        ADC_BUTTON_SPLIT_CHANNEL(button_index);



    uint32_t index =
        ADC_BUTTON_SPLIT_INDEX(button_index);



    int ch_index =
        find_channel(ch);



    if(ch_index < 0)
        return 0;



    uint32_t voltage =
        get_adc_voltage(ch);



    if(
        voltage <= g_button.ch[ch_index].btns[index].max &&
        voltage > g_button.ch[ch_index].btns[index].min
    )
    {

        return 1;

    }



    return 0;

}





esp_err_t button_adc_deinit(adc1_channel_t channel,
                            int button_index)
{

    return ESP_OK;

}