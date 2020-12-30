#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/hw_timer.h>
#include <esp_timer.h>
#include "esp_log.h"
#include <fr_radio.h>

static const char *TAG = "RADIO";

/*
const char *super_star_codes[] = {
    "0000010100000100000101000", // 1 off
    "0100010100000100000101010", // 1 on
    "0000010100010000000101000", // 2 off
    "0100010100010000000101010", // 2 on
    "0000010100010100000101000", // 3 off
    "0100010100010100000101010", // 3 on
};

void send_radio_superstar(const char *sscodestr)
{
    int index = 0;
    
    switch (sscodestr[0]) {
    case '1':
        index = 0;
        break;
    case '2':
        index = 2;
        break;
    case '3':
        index = 4;
        break;
    default:
        return;
    }
    if (sscodestr[1] == 'N') {
        index++;
    }
    send_radio(super_star_codes[index], true);
}
*/

#define MAX_COMMAND_LEN 10

typedef struct {
    uint32_t code;
    int len;
} radio_code_t;

static radio_code_t s_code;

static void clear_code(void)
{
    s_code.code = 0;
    s_code.len = 0;
}

static void append_code(const char *codestr)
{
    int numbits = strlen(codestr);
    int i;
    ESP_LOGI(TAG, "Appending %s to code", codestr);
    for (i = 0; i < numbits; i++) {
        s_code.code <<= 1;
        if (codestr[i] == '1') {
            s_code.code |= 1;
        }
        s_code.len++;
    }
    ESP_LOGI(TAG, "Code is now 0x%08x, length %d", s_code.code, s_code.len);
}

static void build_code_casto(const char *command)
{
    clear_code();

    switch (command[0]) {
        case 'A':
            append_code("00010101");
            break;
        case 'B':
            append_code("01000101");
            break;
        case 'C':
            append_code("01010001");
            break;
        case 'D':
            append_code("01010100");
            break;
    }
    switch (command[1]) {
        case '1':
            append_code("00010101010101");
            break;
        case '2':
            append_code("01000101010101");
            break;
        case '3':
            append_code("01010001010101");
            break;
    }
    switch (command[2]) {
        case 'F':
            append_code("00");
            break;
        case 'N':
            append_code("11");
            break;
    }
}

#define HIGH 1
#define LOW 0

static int phase;
static int bit;
static int count;

esp_timer_handle_t s_timer_handle;
gpio_num_t s_gpio_num;

void set(bool value, uint32_t usec)
{
    if (usec > 0) {
        ESP_ERROR_CHECK(hw_timer_alarm_us(usec, false));
    }
    gpio_set_level(s_gpio_num, value);
}

#define BIT_TIME 200LL
#define BIT_TIME_SHORT (BIT_TIME * 4)
#define BIT_TIME_LONG  (BIT_TIME * 12)
#define BIT_TIME_SYNC  (BIT_TIME * 128)

static void hw_timer_callback(void *args)
{
    if (count > 0) {
      if (bit >= 0) {
        if ((s_code.code & (1 << bit)) == 0) {
            if (phase == 0) {
                phase = 1;
                set(HIGH, BIT_TIME_SHORT);
            } else {
                phase = 0;
                bit--;
                set(LOW, BIT_TIME_LONG);
            }
        } else { 
            if (phase == 0) {
                phase = 1;
                set(HIGH, BIT_TIME_LONG);
            } else {
                phase = 0;
                bit--;
                set(LOW, BIT_TIME_SHORT);
            }
        }
      } else if (bit == -1) {
        if (phase == 0) {
            phase = 1;
            set (HIGH, BIT_TIME_SHORT);
        } else {
            phase = 0;
            bit = s_code.len - 1;
            count--;
            set(LOW, BIT_TIME_SYNC);
        }
      }
    }
}

static void send_code(void)
{
    bit = s_code.len - 1;
    phase = 0;
    count = 5;
    set(HIGH, BIT_TIME_SHORT);
}
 
void send_radio_casto(const char *command)
{
    build_code_casto(command);
    send_code();
}

void radio_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;
    ESP_ERROR_CHECK(hw_timer_init(hw_timer_callback, NULL));
    gpio_set_level(gpio_num, LOW);
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "GPIO %d set to output and initialised to %d", gpio_num, LOW);
}
