#include "driver/gpio.h"
#include "esp_event.h"
#include <ds18x20.h>

ESP_EVENT_DECLARE_BASE(DS1820_EVENT_BASE);

typedef enum {
    DS1820_EVENT_SAMPLE
} ds1820_eventid_t;

typedef struct {
    gpio_num_t gpio_num;
    ds18x20_addr_t address;
    float temperature;
} ds1820_sample_t;

typedef void (*cbDS1820Handler_t) (int32_t event_id, ds18x20_addr_t addr, float temperature);

void ds1820_init(gpio_num_t gpio_num, int period_in_secs, int max_sensors, cbDS1820Handler_t callback);
