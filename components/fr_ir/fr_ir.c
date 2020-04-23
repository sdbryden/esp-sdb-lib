/* IR RX Example
  This is an ir rx demo of ir function( NEC CODE ,32 BIT LENGTH)
  This example code is in the Public Domain (or CC0 licensed, at your option.)
  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/ir_rx.h"
#include "fr_ir.h"

static const char *TAG = "IR";

ESP_EVENT_DEFINE_BASE(IR_EVENT_BASE);

#define IR_RX_BUF_LEN 128 // The actual allocated memory size is sizeof(uint32_t)*BUF_LEN. If the allocation is too small, the old infrared data may be overwritten.

/**
 * @brief check whether the ir cmd and addr obey the protocol
 *
 * @param nec_code nec ir code that received
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
static esp_err_t ir_rx_nec_code_check(ir_rx_nec_data_t nec_code)
{

    if ((nec_code.addr1 != ((~nec_code.addr2) & 0xff))) {
        return ESP_FAIL;
    }

    if ((nec_code.cmd1 != ((~nec_code.cmd2) & 0xff))) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ir_rx_task(void *arg)
{
    ir_rx_nec_data_t ir_data;

    while (1) {
        ir_data.val = 0;
        ir_rx_recv_data(&ir_data, 1, portMAX_DELAY);
        ESP_LOGI(TAG, "addr1: 0x%x, addr2: 0x%x, cmd1: 0x%x, cmd2: 0x%x", ir_data.addr1, ir_data.addr2, ir_data.cmd1, ir_data.cmd2);
        esp_event_post(IR_EVENT_BASE, IR_EVENT_RX, &(ir_data.val), sizeof(ir_data.val), 0);
        if (ESP_OK == ir_rx_nec_code_check(ir_data)) {
            ESP_LOGI(TAG, "ir rx nec data:  0x%x", ir_data.cmd1);
        } else {
            ESP_LOGI(TAG, "Non-standard nec infrared protocol");
        }
    }

    vTaskDelete(NULL);
}

static cbIrHandler_t sCallback;
static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    uint32_t val = * (uint32_t *) event_data;
    uint16_t val1, val2;
    val1 = val & 0xffff;
    val2 = (val >> 16) & 0xffff;
    ESP_LOGI(TAG, "Event %d triggered with val %04x/%04x", id, val1, val2);
    sCallback(id, val1, val2);
}


void ir_recv_init(gpio_num_t gpio_num, cbIrHandler_t callback)
{
    ir_rx_config_t ir_rx_config;
    sCallback = callback;
    esp_err_t rv = esp_event_handler_register(IR_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL);
    ESP_LOGI(TAG, "esp_event_handler_register returned %d", rv);

    ir_rx_config.io_num = gpio_num;
    ir_rx_config.buf_len = IR_RX_BUF_LEN;
    rv = ir_rx_init(&ir_rx_config);
    ESP_LOGI(TAG, "ir_rx_init returned %d", rv);

    xTaskCreate(ir_rx_task, "ir_rx_task", 2048, NULL, 5, NULL);
}
