#include "driver/gpio.h"
#include "esp_event.h"
#include <driver/ir_rx.h>

ESP_EVENT_DECLARE_BASE(IR_EVENT_BASE);

typedef enum {
    IR_EVENT_RX
} ir_eventid_t;

typedef void (*cbIrHandler_t) (int32_t event_id, uint16_t val1, uint16_t val2);

void ir_recv_init(gpio_num_t gpio_num, cbIrHandler_t callback);
