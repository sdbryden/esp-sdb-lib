#include "esp_event.h"
#include <driver/adc.h>

ESP_EVENT_DECLARE_BASE(ANALOG_EVENT_BASE);

typedef enum {
    ANALOG_EVENT_SAMPLE
} analog_eventid_t;

typedef void (*cbAnalogHandler_t) (int32_t event_id, uint16_t value);

void analog_init(int period_in_secs, cbAnalogHandler_t callback);