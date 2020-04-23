#include "esp_event.h"
#include "driver/gpio.h"

ESP_EVENT_DECLARE_BASE(DIG_IN_EVENT_BASE);

typedef enum {
    DIG_IN_EVENT_PRESSED,
    DIG_IN_EVENT_ACTIVE,
    DIG_IN_EVENT_INACTIVE
} dig_in_eventid_t;

typedef struct {
    uint8_t in_use:1;
    uint8_t momentary:1;
    uint8_t active_state:1;
    uint8_t waiting_debounce:1;
} digin_status_t;

typedef void (*cbDiginHandler_t) (int32_t id, gpio_num_t gpio_num);

void digin_init(cbDiginHandler_t callback);
void digin_register_pushbutton(gpio_num_t gpio_num);
void digin_register_on_off(gpio_num_t gpio_num);
void digin_set_active_high(gpio_num_t gpio_num);
void digin_dump_levels(void);