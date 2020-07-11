/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_hidd_prf_api.h"
#include "esp_gap_ble_api.h"

#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

#define TAG "ESP32_KBM"

static bool sec_conn = false;

/* kbm_task() has instructions to what to do when a secure connection is established */
void kbm_task(void *pvParameters);
/* hidd_event_callback() handles ble events */
static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);
/* gap_event_handler() handles GAP events */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void app_main(void)
{
    esp_err_t ret;

    // Initialise NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    if((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(TAG, "%s init bluedroid failed\n", __func__);
    }

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IN;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you, 
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    xTaskCreate(&kbm_task, "hid_task", 2048, NULL, 5, NULL);
}

void kbm_task(void *pvParameters) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (sec_conn) {
        
        }
    }
}

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param) {

}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    
}
