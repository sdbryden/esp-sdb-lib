#include "driver/gpio.h"
#include <string.h>

void radio_init(gpio_num_t gpio_num);
void send_radio_casto(const char *command);
