#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"

#define BUZZER_GPIO 21
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_DUTY (512) // 50% of 10-bit (1024)
#define BUZZER_TICK_US (50 * 1000)

// Note frequencies (Hz)
#define NOTE_E7 2637
#define NOTE_C7 2093
#define NOTE_G7 3136
#define NOTE_G6 1568
#define NOTE_E6 1319
#define NOTE_A6 1760
#define NOTE_B6 1976
#define NOTE_AS6 1865
#define NOTE_F7 2794
#define NOTE_D7 2349
#define NOTE_A7 3520

#define NOTE_G3 196
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

typedef struct
{
    uint16_t frequency;
    uint8_t ticks;
} note_t;

typedef struct
{
    const note_t *score;
    uint8_t length;
    uint8_t index;
    uint8_t ticks_left;
    bool playing;
} buzzer_player_t;

static const note_t melody_baby_shark[] = {
    // Основний мотив
    {NOTE_C4, 10},
    {NOTE_D4, 10},
    {NOTE_E4, 10},

    {NOTE_C4, 10},
    {NOTE_D4, 10},
    {NOTE_E4, 10},

    {NOTE_C4, 10},
    {NOTE_D4, 10},
    {NOTE_E4, 10},

    {NOTE_E4, 10},
    {NOTE_D4, 10},
    {NOTE_C4, 20},

    // baby shark
    {NOTE_C4, 5},
    {NOTE_D4, 5},
    {NOTE_E4, 10},

    {NOTE_C4, 5},
    {NOTE_D4, 5},
    {NOTE_E4, 10},

    {NOTE_C4, 5},
    {NOTE_D4, 5},
    {NOTE_E4, 10},

    {NOTE_E4, 5},
    {NOTE_D4, 5},
    {NOTE_C4, 20},

    // End
    {NOTE_C4, 10},
    {NOTE_D4, 10},
    {NOTE_E4, 10},

    {NOTE_C4, 10},
    {NOTE_D4, 10},
    {NOTE_E4, 10},

    {NOTE_C4, 10},
    {NOTE_D4, 10},
    {NOTE_E4, 10},

    {NOTE_C4, 40},
};
static buzzer_player_t player;

static void buzzer_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = 2000, // default, will be changed per note
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&channel_config);
}
static void buzzer_play(const note_t *score, uint8_t length)
{
    player.score = score;
    player.length = length;
    player.index = 0;
    player.ticks_left = 0;
    player.playing = true;
}
static void buzzer_player_tick(void)
{
    if (!player.playing)
        return;

    if (player.ticks_left == 0)
    {
        if (player.index >= player.length)
        {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            player.playing = false;
            return;
        }

        note_t melody = player.score[++player.index];

        if (melody.frequency == 0)
        {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
        }
        else
        {
            ledc_set_freq(LEDC_MODE, LEDC_TIMER, melody.frequency);
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
        }

        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
        player.ticks_left = melody.ticks;
    }
    player.ticks_left--;
}

void app_main(void)
{
    buzzer_init();
    buzzer_play(
        melody_baby_shark,
        sizeof(melody_baby_shark) / sizeof(note_t));

    int64_t last_tick = esp_timer_get_time();

    while (1)
    {
        int64_t now = esp_timer_get_time();
        if (now - last_tick >= BUZZER_TICK_US)
        {
            last_tick += BUZZER_TICK_US;
            buzzer_player_tick();
        }
    }
}