#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

#define TAG "ADC_SMA"

#define SMA_WINDOW_SIZE 16

#define ADC_UNIT_USED ADC_UNIT_1
#define ADC_CHANNEL_USED ADC_CHANNEL_3
#define ADC_ATTEN_USED ADC_ATTEN_DB_12
#define ADC_BITWIDTH_USED ADC_BITWIDTH_12

#define LED_GPIO GPIO_NUM_11

static bool led_state = false;
static int sma_buffer[SMA_WINDOW_SIZE];
static int sma_index = 0;
static int sma_count = 0;
static int sma_sum = 0;

adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
bool cali_enabled = false;

static int sma_add_sample(int new_sample)
{
    // Remove oldest sample if buffer is full
    if (sma_count == SMA_WINDOW_SIZE)
    {
        sma_sum -= sma_buffer[sma_index];
    }
    else
    {
        sma_count++;
    }

    // Add new sample
    sma_buffer[sma_index] = new_sample;
    sma_sum += new_sample;

    // Advance circular index
    sma_index++;
    if (sma_index >= SMA_WINDOW_SIZE)
    {
        sma_index = 0;
    }

    return sma_sum / sma_count;
}

void gpio_init(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&led_conf);
}

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_USED};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(
        &init_config,
        &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle,
        ADC_CHANNEL_3,
        &chan_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_USED,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle) == ESP_OK)
    {
        cali_enabled = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    }
    else
    {
        ESP_LOGI(TAG, "ADC calibration is not enabled");
    }
}

void adc_read_voltage(void)
{

    int u_cali = 0;
    int raw = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &raw));

    if (cali_enabled)
    {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &u_cali));
    }
    else
    {
        u_cali = raw;
    }

    int averaged_value = sma_add_sample(u_cali);

    ESP_LOGI(
        TAG,
        "Raw: %4d | SMA: %4d | U_CALI: %4d",
        raw,
        averaged_value,
        u_cali);

    if (!led_state && averaged_value > 2500)
    {
        gpio_set_level(LED_GPIO, 1);
        led_state = true;
    }
    else if (led_state && averaged_value < 1700)
    {
        gpio_set_level(LED_GPIO, 0);
        led_state = false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

void app_main(void)
{
    gpio_init();
    adc_init();

    while (1)
    {
        adc_read_voltage();
    }
}