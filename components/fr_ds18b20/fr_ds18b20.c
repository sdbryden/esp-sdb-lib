#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include <ds18x20.h>
#include "fr_ds18b20.h"

static const char *TAG = "DS1820";

ESP_EVENT_DEFINE_BASE(DS1820_EVENT_BASE);

static const int RESCAN_INTERVAL = 8;
static struct config {
    gpio_num_t gpio_num;
    int max_sensors;
    int period_ms;
} config;


static void ds1820_read_all(void *pvParameter)
{
    int sensor_count;
    struct config *config = (struct config *) pvParameter;
    ds18x20_addr_t addrs[config->max_sensors];
    float temps[config->max_sensors];
    ds1820_sample_t sample;

    // There is no special initialization required before using the ds18x20
    // routines.  However, we make sure that the internal pull-up resistor is
    // enabled on the GPIO pin so that one can connect up a sensor without
    // needing an external pull-up (Note: The internal (~47k) pull-ups of the
    // ESP8266 do appear to work, at least for simple setups (one or two sensors
    // connected with short leads), but do not technically meet the pull-up
    // requirements from the ds18x20 datasheet and may not always be reliable.
    // For a real application, a proper 4.7k external pull-up resistor is
    // recommended instead!)

    while (1)
    {
        // Every RESCAN_INTERVAL samples, check to see if the sensors connected
        // to our bus have changed.
        sensor_count = ds18x20_scan_devices(config->gpio_num, addrs, config->max_sensors);

        if (sensor_count < 1) {
            ESP_LOGI(TAG, "No sensors detected!");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        } else {
            ESP_LOGI(TAG, "%d sensors detected:", sensor_count);
            // If there were more sensors found than we have space to handle,
            // just report the first config->max_sensors..
            if (sensor_count > config->max_sensors)
                sensor_count = config->max_sensors;

            // Do a number of temperature samples, and print the results.
            for (int i = 0; i < RESCAN_INTERVAL; i++)
            {
                ds18x20_measure_and_read_multi(config->gpio_num, addrs, sensor_count, temps);
                for (int j = 0; j < sensor_count; j++)
                {
                    sample.gpio_num = config->gpio_num;
                    sample.address = addrs[j];
                    sample.temperature = temps[j];
                    esp_event_post(DS1820_EVENT_BASE, DS1820_EVENT_SAMPLE, &sample, sizeof(sample), 0);
                }
                printf("\n");

                // Wait for a little bit between each sample (note that the
                // ds18x20_measure_and_read_multi operation already takes at
                // least 750ms to run, so this is on top of that delay).
                vTaskDelay(config->period_ms / portTICK_PERIOD_MS);
            }
        }
    }
}

// FIXME This function is dangerous. Needs to be fixed.
static char *ftoa(char *a, double f, int precision)
{
    long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
    
    char *ret = a;
    long heiltal = (long)f;
    itoa(heiltal, a, 10);
    while (*a != '\0') {
        a++;
    }
    *a++ = '.';
    long desimal = abs((long)((f - heiltal) * p[precision]));
    itoa(desimal, a, 10);
    return ret;
}

static cbDS1820Handler_t sCallback;
static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    ds1820_sample_t *sample = (ds1820_sample_t *) event_data;
    static char tempstr[10];
    ESP_LOGI(TAG, "Event %d triggered", id);
    ftoa(tempstr, sample->temperature, 1);
    ESP_LOGI(TAG, "Sensor %08x reports %s deg C", ((uint32_t) (sample->address & 0xffffffff)), tempstr);
    sCallback(id, sample->address, sample->temperature, tempstr);
}

void ds1820_init(gpio_num_t gpio_num, int period_in_secs, int max_sensors, bool enable_pullup, cbDS1820Handler_t callback)
{
    sCallback = callback;
    config.gpio_num = gpio_num;
    config.max_sensors = max_sensors;
    config.period_ms = period_in_secs * 1000;

    if (enable_pullup) {
        gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    }

    xTaskCreate(ds1820_read_all, "ds1820_read", configMINIMAL_STACK_SIZE * 10, &config, 5, NULL);
    esp_err_t rv = esp_event_handler_register(DS1820_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL);
    ESP_LOGI(TAG, "esp_event_handler_register returned %d", rv);
}
