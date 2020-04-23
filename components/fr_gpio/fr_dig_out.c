#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "fr_dig_out.h"

static const char *TAG = "DIG_OUT";

#define NUM_DIGOUTS 17
static dig_out_t dig_outs[NUM_DIGOUTS];

static dig_out_t *digout_get_status(gpio_num_t gpio_num)
{
    if (gpio_num >= NUM_DIGOUTS) {
        ESP_LOGE(TAG, "GPIO number out of range");
    }
    return &dig_outs[gpio_num];
}

void digout_init_pin(gpio_num_t gpio_num, bool level)
{
    digout_set(gpio_num, level);
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "GPIO %d set to output and initialised to %d", gpio_num, level);
}

void digout_set(gpio_num_t gpio_num, bool level)
{
    dig_out_t *d = digout_get_status(gpio_num);
    d->return_level = level;
    d->level = level;
    gpio_set_level(gpio_num, level);
    ESP_LOGI(TAG, "Digital output %d set to %d", gpio_num, level);
}

bool digout_get(gpio_num_t gpio_num)
{
    dig_out_t *d = digout_get_status(gpio_num);
    return d->level;
}

static void vTimerCallback(TimerHandle_t xTimer)
{
    dig_out_t *d;
    int i;
    for (i = 0; i < NUM_DIGOUTS; i++) {
        d = &dig_outs[i];
        if (d->expiry > 0) {
            d->expiry--;
            if (d->expiry == 0) {
                digout_set(i, d->return_level);
                ESP_LOGI(TAG, "Digital output %d timer expired. Returning to to %d", i, d->return_level);
            }
        }
    }
}

void digout_set_halfsecs(gpio_num_t gpio_num, bool level, int halfsecs)
{
    dig_out_t *d = digout_get_status(gpio_num);
    d->expiry = halfsecs;
    d->level = level;
    gpio_set_level(gpio_num, level);
    ESP_LOGI(TAG, "Digital output %d set to %d for %d halfsecs", gpio_num, level, halfsecs);
}

void digout_init(void)
{
    TimerHandle_t xTimer;
    xTimer = xTimerCreate("DigOutTimer", pdMS_TO_TICKS(500), pdTRUE, NULL, vTimerCallback);
    if (xTimer) {
        xTimerStart(xTimer, 0);
    }
    ESP_LOGI(TAG, "Digital output module started");
}
