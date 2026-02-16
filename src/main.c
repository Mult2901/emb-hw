#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define TAG "ADC"

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

#define ADC_UNIT_USED ADC_UNIT_1
#define ADC_CHANNEL_USED ADC_CHANNEL_3
#define ADC_CHANNLE_USED_4 ADC_CHANNEL_4
#define ADC_ATTEN_USED ADC_ATTEN_DB_12
#define ADC_BITWIDTH_USED ADC_BITWIDTH_12

#define LEDC_TIMER_LED 0
#define LED_PWM_PIN 18
#define LEDC_CHANNEL_LED 1
#define LEDC_FREQUENCY_LED 1000

#define LEDC_TIMER_MOTOR 1
#define MOTOR_PWM_PIN 19
#define LEDC_CHANNEL_MOTOR 2
#define LEDC_FREQUENCY_MOTOR 20000

void timer_init(void)
{
    ledc_timer_config_t led_timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_LED,
        .freq_hz = LEDC_FREQUENCY_LED,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer_config));

    ledc_timer_config_t motor_timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_MOTOR,
        .freq_hz = LEDC_FREQUENCY_MOTOR,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&motor_timer_config));

    ledc_channel_config_t led_channel_config = {
        .gpio_num = LED_PWM_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_LED,
        .timer_sel = LEDC_TIMER_LED,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel_config));

    ledc_channel_config_t motor_channel_config = {
        .gpio_num = MOTOR_PWM_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_MOTOR,
        .timer_sel = LEDC_TIMER_MOTOR,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&motor_channel_config));
}
void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_USED,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_USED,
        .bitwidth = ADC_BITWIDTH_USED,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle,
        ADC_CHANNEL_USED,
        &channel_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_USED,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
        &cali_config,
        &cali_handle));
}
void app_main(void)
{
    timer_init();
    adc_init();

    while (1)
    {
        int raw_adc_value = 0;
        int u_cali = 0;

        ESP_ERROR_CHECK(adc_oneshot_read(
            adc_handle,
            ADC_CHANNEL_USED,
            &raw_adc_value));

        if (cali_handle)
        {
            ESP_ERROR_CHECK(
                adc_cali_raw_to_voltage(cali_handle, raw_adc_value, &u_cali));
        }
        else
        {
            u_cali = raw_adc_value;
        }

        int duty_cycle = (u_cali * 1023) / 3100;

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_LED, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_LED);

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_MOTOR, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_MOTOR);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}