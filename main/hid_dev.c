// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "hid_dev.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_log.h"

static hid_report_map_t *hid_dev_rpt_tbl;
static uint8_t hid_dev_rpt_tbl_Len;

static hid_report_map_t *hid_dev_rpt_by_id(uint8_t id, uint8_t type)
{
    hid_report_map_t *rpt = hid_dev_rpt_tbl;

    for (uint8_t i = hid_dev_rpt_tbl_Len; i > 0; i--, rpt++)
    {
        if (rpt->id == id && rpt->type == type && rpt->mode == hidProtocolMode)
        {
            return rpt;
        }
    }

    return NULL;
}

static hid_report_map_t *hid_dev_boot_rpt_by_id(uint8_t id, uint8_t type)
{
    hid_report_map_t *rpt = hid_dev_rpt_tbl;

    for (uint8_t i = hid_dev_rpt_tbl_Len; i > 0; i--, rpt++)
    {
        if (rpt->id == id && rpt->type == type && rpt->mode == hidBootMode)
        {
            return rpt;
        }
    }

    return NULL;
}

void hid_dev_register_reports(uint8_t num_reports, hid_report_map_t *p_report)
{
    hid_dev_rpt_tbl = p_report;
    hid_dev_rpt_tbl_Len = num_reports;
    return;
}

void hid_dev_send_report(esp_gatt_if_t gatts_if, uint16_t conn_id,
                         uint8_t id, uint8_t type, uint8_t length, uint8_t *data)
{
    hid_report_map_t *p_rpt;

    // get att handle for report
    if ((p_rpt = hid_dev_rpt_by_id(id, type)) != NULL)
    {
        // if notifications are enabled
        ESP_LOGD(HID_LE_PRF_TAG, "%s(), send the report, handle = %d", __func__, p_rpt->handle);
        esp_ble_gatts_send_indicate(gatts_if, conn_id, p_rpt->handle, length, data, false);
    }

    return;
}

void enable_led_notifications() {
    hid_report_map_t *p_rpt;
    uint16_t length = 1;

    uint8_t value = 1;
    esp_err_t ret;
    if ((p_rpt = hid_dev_rpt_by_id(HID_RPT_ID_LED_OUT, HID_REPORT_TYPE_OUTPUT)) != NULL) {
        ret = esp_ble_gatts_set_attr_value(p_rpt->cccdHandle, length, &value);
        if (ret != ESP_OK) {
            ESP_LOGE(HID_LE_PRF_TAG, "%s(), Failed to enable notifications", __func__);
        } else {
            ESP_LOGI(HID_LE_PRF_TAG, "%s(), Enabled notifications", __func__);
        }
    } else {
        ESP_LOGI(HID_LE_PRF_TAG, "%s(), Report does not exist", __func__);
    }
}

void disable_led_notifications() {
    hid_report_map_t *p_rpt;
    uint16_t length = 1;

    uint8_t value = 0;
    esp_err_t ret;
    if ((p_rpt = hid_dev_rpt_by_id(HID_RPT_ID_LED_OUT, HID_REPORT_TYPE_OUTPUT)) != NULL) {
        ret = esp_ble_gatts_set_attr_value(p_rpt->cccdHandle, length, &value);
        if (ret != ESP_OK) {
            ESP_LOGE(HID_LE_PRF_TAG, "%s(), Failed to enable notifications", __func__);
        } else {
            ESP_LOGI(HID_LE_PRF_TAG, "%s(), Enabled notifications", __func__);
        }
    } else {
        ESP_LOGI(HID_LE_PRF_TAG, "%s(), Report does not exist", __func__);
    }
}

uint8_t hid_dev_get_leds()
{
    hid_report_map_t *p_rpt;
    uint16_t length = 0;
    const uint8_t *value;

    esp_err_t ret;

    ESP_LOGD(HID_LE_PRF_TAG, "Checking report using protocol mode");
    // get att handle for report using protocol mode
    if ((p_rpt = hid_dev_rpt_by_id(HID_RPT_ID_LED_OUT, HID_REPORT_TYPE_OUTPUT)) != NULL)
    {
        ret = esp_ble_gatts_get_attr_value(p_rpt->handle, &length, &value);

        if (ret == ESP_FAIL)
        {
            ESP_LOGE(HID_LE_PRF_TAG, "Illegal handle");
        }
        else
        {
            ESP_LOGD(HID_LE_PRF_TAG, "length: %x\n", length);
            ESP_LOGI(HID_LE_PRF_TAG, "protocol mode: %d", *value);
            if (length > 0)
            {
                return value[0];
            }
        }
    }

    return 0;
}

void hid_consumer_build_report(uint8_t *buffer, consumer_cmd_t cmd)
{
    if (!buffer)
    {
        ESP_LOGE(HID_LE_PRF_TAG, "%s(), the buffer is NULL, hid build report failed.", __func__);
        return;
    }

    switch (cmd)
    {
    case HID_CONSUMER_CHANNEL_UP:
        HID_CC_RPT_SET_CHANNEL(buffer, HID_CC_RPT_CHANNEL_UP);
        break;

    case HID_CONSUMER_CHANNEL_DOWN:
        HID_CC_RPT_SET_CHANNEL(buffer, HID_CC_RPT_CHANNEL_DOWN);
        break;

    case HID_CONSUMER_VOLUME_UP:
        HID_CC_RPT_SET_VOLUME_UP(buffer);
        break;

    case HID_CONSUMER_VOLUME_DOWN:
        HID_CC_RPT_SET_VOLUME_DOWN(buffer);
        break;

    case HID_CONSUMER_MUTE:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_MUTE);
        break;

    case HID_CONSUMER_POWER:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_POWER);
        break;

    case HID_CONSUMER_RECALL_LAST:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_LAST);
        break;

    case HID_CONSUMER_ASSIGN_SEL:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_ASSIGN_SEL);
        break;

    case HID_CONSUMER_PLAY:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_PLAY);
        break;

    case HID_CONSUMER_PAUSE:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_PAUSE);
        break;

    case HID_CONSUMER_RECORD:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_RECORD);
        break;

    case HID_CONSUMER_FAST_FORWARD:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_FAST_FWD);
        break;

    case HID_CONSUMER_REWIND:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_REWIND);
        break;

    case HID_CONSUMER_SCAN_NEXT_TRK:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_SCAN_NEXT_TRK);
        break;

    case HID_CONSUMER_SCAN_PREV_TRK:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_SCAN_PREV_TRK);
        break;

    case HID_CONSUMER_STOP:
        HID_CC_RPT_SET_BUTTON(buffer, HID_CC_RPT_STOP);
        break;

    default:
        break;
    }

    return;
}
