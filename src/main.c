#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define SERVO_GPIO 4

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0

#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define SERVO_PWM_FREQ 50     // 50 Hz
#define SERVO_PERIOD_US 20000 // 20 ms

#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500

static void servo_set_angle(int angle)
{
    // Clamp angle
    if (angle < 0)
        angle = 0;
    if (angle > 180)
        angle = 180;

    // Angle to pulse width (µs)
    uint32_t pulse_us =
        SERVO_MIN_US +
        (angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180;

    // Pulse width to duty
    uint32_t max_duty = (1 << LEDC_DUTY_RES) - 1;
    uint32_t duty = (pulse_us * max_duty) / SERVO_PERIOD_US;

    // Write to LEDC
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void servo_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = SERVO_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_GPIO,
        .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void app_main(void)
{
    servo_init();

    while (1)
    {
        //  forward
        for (int angle = 0; angle <= 180; angle += 1)
        {
            servo_set_angle(angle);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        //  backward
        for (int angle = 180; angle >= 0; angle -= 1)
        {
            servo_set_angle(angle);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}