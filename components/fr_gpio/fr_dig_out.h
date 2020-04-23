#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"

typedef struct dig_out_s {
    uint8_t level:1;
    uint8_t return_level:1;
    uint8_t spare;
    uint16_t expiry;
} dig_out_t;

void digout_init_pin(gpio_num_t gpio_num, bool level);
void digout_set(gpio_num_t gpio_num, bool level);
bool digout_get(gpio_num_t gpio_num);
void digout_set_halfsecs(gpio_num_t gpio_num, bool level, int halfsecs);
void digout_init(void);
