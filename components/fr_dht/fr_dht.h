#include "esp_event.h"
#include <dht.h>

ESP_EVENT_DECLARE_BASE(DHT_EVENT_BASE);

typedef enum {
    DHT_EVENT_SAMPLE
} dht_eventid_t;

typedef struct {
    gpio_num_t gpio_num;
    int16_t temperature;
    int16_t humidity;
} dht_sample_t;

typedef void (*cbDhtHandler_t) (int32_t event_id, int16_t temperature, int16_t humidity, const char *tempstr, const char *humstr);

void dht_init(gpio_num_t gpio_num, int period_in_secs, dht_sensor_type_t type, cbDhtHandler_t callback);