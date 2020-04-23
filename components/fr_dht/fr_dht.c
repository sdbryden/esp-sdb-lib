#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include <dht.h>
#include "fr_dht.h"

static const char *TAG = "DHT";

ESP_EVENT_DEFINE_BASE(DHT_EVENT_BASE);

static struct config {
    gpio_num_t gpio_num;
    int period_ms;
    dht_sensor_type_t type;
} config;


static void dht_read(void *pvParameters)
{
    struct config *config = (struct config *) pvParameters;
    int16_t temperature = 0;
    int16_t humidity = 0;
    dht_sample_t sample;

    // DHT sensors that come mounted on a PCB generally have
    // pull-up resistors on the data pin.  It is recommended
    // to provide an external pull-up resistor otherwise...

    gpio_set_direction(config->gpio_num, GPIO_MODE_INPUT);

    while (1)
    {
        if (dht_read_data(config->type, config->gpio_num, &humidity, &temperature) == ESP_OK) {
            ESP_LOGI(TAG, "Humidity: %d%% Temp: %dC", humidity / 10, temperature / 10);
            sample.gpio_num = config->gpio_num;
            sample.temperature = ((float) temperature) / 10;
            sample.humidity = ((float) humidity) / 10;
            esp_event_post(DHT_EVENT_BASE, DHT_EVENT_SAMPLE, &sample, sizeof(sample), 0);
        } else {
            ESP_LOGI(TAG, "Could not read data from sensor");
        }
        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        vTaskDelay(config->period_ms / portTICK_PERIOD_MS);
    }
}

static cbDhtHandler_t sCallback;
static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    dht_sample_t *sample = (dht_sample_t *) event_data;
    ESP_LOGI(TAG, "Event %d triggered", id);
    sCallback(id, sample->temperature, sample->humidity);
}

void dht_init(gpio_num_t gpio_num, int period_in_secs, dht_sensor_type_t type, cbDhtHandler_t callback)
{
    sCallback = callback;
    config.gpio_num = gpio_num;
    config.type = type;
    config.period_ms = period_in_secs * 1000;
    xTaskCreate(dht_read, "dht_read", configMINIMAL_STACK_SIZE * 5, &config, 5, NULL);
    esp_err_t rv = esp_event_handler_register(DHT_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL);
    ESP_LOGI(TAG, "esp_event_handler_register returned %d", rv);
}
