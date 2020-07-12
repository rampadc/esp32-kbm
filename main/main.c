/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdio.h>
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_console.h"

#include "ble_kbm_types.h"
#include "commands.h"

#define TAG "ESP32_KBM"

extern void initialise_nvs();

extern void initialise_console();
extern void config_prompts();
extern void watch_prompts();
extern void console_register_bluetooth_commands();

extern void initialise_bluetooth();
extern bool has_ble_secure_connection();
extern void bluetooth_send_character(char);
extern void handle_bluetooth_task();

/* hid_task() has instructions to what to do when a secure connection is established */
void hid_task(void *pvParameters);
void console_task(void *pvParameters);

QueueHandle_t passkey_queue, keyboard_queue, mouse_queue, commands_queue;

void app_main(void)
{
    initialise_nvs();

    /* Initialise queues */
    passkey_queue = xQueueCreate(1, sizeof(uint32_t));
    keyboard_queue = xQueueCreate(8, sizeof(keyboard_t));
    mouse_queue = xQueueCreate(8, sizeof(mouse_t));
    commands_queue = xQueueCreate(1, sizeof(uint8_t));

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
    xTaskCreate(&console_task, "console_task", 4096, NULL, 4, NULL);
}

void hid_task(void *pvParameters)
{
    handle_bluetooth_task();
    vTaskDelete(NULL);
}

void console_task(void *pvParameters)
{
    watch_prompts();
    vTaskDelete(NULL);
}