#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include "fr_analog.h"

static const char *TAG = "ANALOG";

ESP_EVENT_DEFINE_BASE(ANALOG_EVENT_BASE);

static struct config {
    gpio_num_t gpio_num;
    int period_ms;
} config;


static void analog_read(void *pvParameters)
{
    struct config *config = (struct config *) pvParameters;
    uint16_t value = 0;
    adc_config_t adc_config;

    gpio_set_direction(config->gpio_num, GPIO_MODE_INPUT);

    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 32;
    adc_init(&adc_config);

    while (1)
    {
        if (adc_read(&value) == ESP_OK) {
            ESP_LOGI(TAG, "ADC: %d", value);
            esp_event_post(ANALOG_EVENT_BASE, ANALOG_EVENT_SAMPLE, &value, sizeof(value), 0);
        } else {
            ESP_LOGE(TAG, "Could not read data from ADC");
        }
        vTaskDelay(config->period_ms / portTICK_PERIOD_MS);
    }
}

static cbAnalogHandler_t sCallback;
static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    uint16_t value = *(uint16_t *) event_data;
    ESP_LOGI(TAG, "Event %d triggered, value %d", id, value);
    sCallback(id, value);
}

void analog_init(int period_in_secs, cbAnalogHandler_t callback)
{
    sCallback = callback;
    config.gpio_num = 0; // not used for 8266, as there is only one ADC
    config.period_ms = period_in_secs * 1000;
    xTaskCreate(analog_read, "analog_read", configMINIMAL_STACK_SIZE * 5, &config, 5, NULL);
    esp_err_t rv = esp_event_handler_register(ANALOG_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL);
    ESP_LOGI(TAG, "esp_event_handler_register returned %d", rv);
}
