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
#include "esp_console.h"

#define TAG "ESP32_KBM"

extern void initialise_nvs();

extern void initialise_console();
extern void config_prompts();
extern void watch_prompts();
extern void console_register_bluetooth_commands();

extern void initialise_bluetooth();
extern bool has_ble_secure_connection();

/* hid_task() has instructions to what to do when a secure connection is established */
void hid_task(void *pvParameters);
void console_task(void *pvParameters);

void app_main(void)
{
    initialise_nvs();

    initialise_bluetooth();

    initialise_console();
    config_prompts();
    /* Register console commands */
    esp_console_register_help_command();
    console_register_bluetooth_commands();

    /* 
        Low priority numbers denote low priority tasks. The idle task has priority zero (tskIDLE_PRIORITY). 
        https://www.freertos.org/RTOS-task-priority.html
    */
    xTaskCreate(&hid_task, "hid_task", 2048, NULL, 5, NULL);
    xTaskCreate(&console_task, "console_task", 2048, NULL, 4, NULL);
}

void hid_task(void *pvParameters)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while (1)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (has_ble_secure_connection())
        {
        }
    }
}

void console_task(void *pvParameters)
{
    watch_prompts();
}