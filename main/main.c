/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include "esp_spi_flash.h"
#include "esp_err.h"

#define TAG "ESP32_KBM"

/* hid_task() has instructions to what to do when a secure connection is established */
void hid_task(void *pvParameters);

extern void initialise_bluetooth();
extern bool sec_conn;

extern void initialise_nvs();

void app_main(void)
{
    initialise_nvs();
    initialise_bluetooth();

    xTaskCreate(&hid_task, "hid_task", 2048, NULL, 5, NULL);
}

void hid_task(void *pvParameters) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (sec_conn) {
        
        }
    }
}