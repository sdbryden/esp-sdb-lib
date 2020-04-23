#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "fr_dig_in.h"

ESP_EVENT_DEFINE_BASE(DIG_IN_EVENT_BASE);

#define NUM_DIGINS 17

static const char *TAG = "DIG_IN";
static QueueHandle_t tsqueue = NULL;
static digin_status_t digin_status[NUM_DIGINS];

static void set_gpio_config(gpio_num_t gpio_num, gpio_mode_t mode, gpio_int_type_t int_type, bool pullup, bool pulldown)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = int_type;
    //set as output mode
    io_conf.mode = mode;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = 1 << gpio_num;
    //disable pull-down mode
    io_conf.pull_down_en = pulldown;
    //disable pull-up mode
    io_conf.pull_up_en = pullup;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

}
static digin_status_t *digin_get_status(gpio_num_t gpio_num)
{
    if (gpio_num >= NUM_DIGINS) {
        ESP_LOGE(TAG, "GPIO number out of range");
    }
    return &digin_status[gpio_num];
}

static void digin_handler(void *arg)
{
    gpio_num_t gpio_num = (gpio_num_t) arg;
    xQueueSendFromISR(tsqueue, &gpio_num, NULL);   
}

void digin_register_pushbutton(gpio_num_t gpio_num)
{
    digin_status_t *status = digin_get_status(gpio_num);
    status->in_use = 1;
    status->momentary = 1;
    set_gpio_config(gpio_num, GPIO_MODE_INPUT, GPIO_INTR_NEGEDGE, true, false);
    gpio_isr_handler_add(gpio_num, digin_handler, (void *) gpio_num);
}

void digin_register_on_off(gpio_num_t gpio_num)
{
    digin_status_t *status = digin_get_status(gpio_num);
    status->in_use = 1;
    ESP_LOGI(TAG, "Setting on_off for gpio %d", gpio_num);
    set_gpio_config(gpio_num, GPIO_MODE_INPUT, GPIO_INTR_ANYEDGE, true, false);
    gpio_isr_handler_add(gpio_num, digin_handler, (void *) gpio_num);
}

void digin_set_active_high(gpio_num_t gpio_num)
{
    digin_status_t *status = digin_get_status(gpio_num);
    status->active_state = 1;
}

void digin_dump_levels(void)
{
    int i;
    digin_status_t *status;;
    for (i = 0; i < NUM_DIGINS; i++) {
        status = digin_get_status(i);
        if (status->in_use) {
            printf("Level of GPIO %d is %d ", i, gpio_get_level(i));
            printf("Momentary: %d, Active_state: %d, Waiting: %d\n", status->momentary, status->active_state, status->waiting_debounce);
        }
    }
}

static void vTimerCallback(TimerHandle_t xTimer)
{
    gpio_num_t gpio_num = (gpio_num_t) pvTimerGetTimerID(xTimer);
    digin_status_t *status = digin_get_status(gpio_num);
    ESP_LOGI(TAG, "In digin timer callback, gpio %d", gpio_num);
    if (status->momentary) {
        if (gpio_get_level(gpio_num) == status->active_state) {
            ESP_LOGI(TAG, "Button %d pressed and debounced", gpio_num);
            esp_err_t rv = esp_event_post(DIG_IN_EVENT_BASE, DIG_IN_EVENT_PRESSED, &gpio_num, sizeof(gpio_num), 0);
            ESP_LOGI(TAG, "esp_event_post returned %d", rv);
        }
    } else {
        if (gpio_get_level(gpio_num) == status->active_state) {
            ESP_LOGI(TAG, "Digital input %d active", gpio_num);
            esp_err_t rv = esp_event_post(DIG_IN_EVENT_BASE, DIG_IN_EVENT_ACTIVE, &gpio_num, sizeof(gpio_num), 0);
            ESP_LOGI(TAG, "esp_event_post returned %d", rv);
        } else {
            ESP_LOGI(TAG, "Digital input %d inactive", gpio_num);
            esp_err_t rv = esp_event_post(DIG_IN_EVENT_BASE, DIG_IN_EVENT_INACTIVE, &gpio_num, sizeof(gpio_num), 0);
            ESP_LOGI(TAG, "esp_event_post returned %d", rv);
        }
    }
    xTimerDelete(xTimer, 0);
    status->waiting_debounce = 0;
}

static void buttonIntTask(void *pvParameters)
{
    TimerHandle_t xTimer;
    gpio_num_t gpio_num;
    
    while(1) {
        ESP_LOGI(TAG, "Waiting for button press interrupt");
        xQueueReceive(tsqueue, &gpio_num, portMAX_DELAY);
        ESP_LOGI(TAG, "ISR signalled main task, gpio %d", gpio_num);
        digin_status_t *status = digin_get_status(gpio_num);
        if (status->waiting_debounce == 0) {
            ESP_LOGI(TAG, "Digital input %d changed state", gpio_num);
            status->waiting_debounce = 1;
            xTimer = xTimerCreate("Timer", pdMS_TO_TICKS(40), pdFALSE, (void *) gpio_num, vTimerCallback);
            if (xTimer) {
                xTimerStart(xTimer, 0);
            }
        } else {
            ESP_LOGI(TAG, "Digital input %d bounced", gpio_num);
        }
    }
}

/*
void gpio_intr_handler(uint8_t gpio_num)
{
    uint32_t now = xTaskGetTickCountFromISR();
    xQueueSendToBackFromISR(tsqueue, &now, NULL);
}
*/

static cbDiginHandler_t sCallback;
void event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    gpio_num_t gpio_num = *(gpio_num_t *) event_data;
    ESP_LOGI(TAG, "Event %d triggered: GPIO %d", id, gpio_num);
    sCallback(id, gpio_num);
}

void digin_init(cbDiginHandler_t callback)
{
    sCallback = callback;
    gpio_install_isr_service(0);
    tsqueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(buttonIntTask, "buttonIntTask", 2560, &tsqueue, 20, NULL);
    esp_err_t rv = esp_event_handler_register(DIG_IN_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL);
    ESP_LOGI(TAG, "esp_event_handler_register returned %d", rv);
}
