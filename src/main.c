#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "ADC";

static adc_oneshot_unit_handle_t adc_handle;

static adc_cali_handle_t cali_handle;
static adc_bitwidth_t ADC_BITWIDTH = ADC_BITWIDTH_12;
static adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_12;
static adc_unit_t ADC_UNIT = ADC_UNIT_1;
static adc_channel_t ADC_CHANNEL = ADC_CHANNEL_3; // Використовуємо канал 3 (GPIO4 на ESP32)
bool cali_enabled = false;

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTENUATION,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITWIDTH,
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
    int raw = 0;
    int u_cali = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
    int u_manual = (raw * 3300) / 4095;

    if (cali_enabled)
    {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &u_cali));
    }

    float error = 0.0f;
    if (u_cali > 0)
    {
        error = ((float)(u_manual - u_cali) / u_cali) * 100.0f;
    }

    ESP_LOGI(TAG, "%4d\t%4d\t%4d\t%.2f",
             raw, u_manual, u_cali, error);
}

void app_main(void)
{
    adc_init();

    ESP_LOGI(TAG, "RAW\tU_manual(mV)\tU_cali(mV)\t Error(%%)");
    ESP_LOGI(TAG, "------------------------------------------");

    while (1)
    {
        adc_read_voltage();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}