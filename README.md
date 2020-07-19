# ESP32-KBM Project Overview

This project aims to be the most complete ESP32 Bluetooth keyboard and mouse implementation out there with the following features:

1. Support keyboard
2. Support mouse
3. Support LED outputs
4. Support consumer reports
5. Low power mode
6. Maintainable console input and output using [Espressif's console component](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/console.html)
7. Security must has input capability, mitm, encrypted, bonding support.

## Status

Abandoning for simpler implementation with arduino-esp32. Neil Kolban implemented much of the LED output report codes already. This keyboard-only [example](https://gist.github.com/manuelbl/66f059effc8a7be148adb1f104666467) shows how this can be done. It's possible to do the same thing with the report map used in ESP-IDF v4.0 HID keyboard and mouse example.

## How is it different from other existing projects?

- ESP-IDF v4.0 includes a BLE keyboard and mouse [demo](https://github.com/espressif/esp-idf/tree/release/v4.0/examples/bluetooth/bluedroid/ble/ble_hid_device_demo) out-of-the-box. It has support for points 1, 2, 4.
- [esp32_mouse_keyboard](https://github.com/asterics/esp32_mouse_keyboar) project is based on the example provided by Espressif. It has support for points 1, 2, 4 with a partial 6. While this project has UART console support, it does not use the console library.

## Progress

Based on the demo project, this project did some refactoring by breaking out the console and bluetooth [tasks](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/freertos-smp.html#tasks-and-task-creation) into their own files and use queues for communication ([commit](https://github.com/rampadc/esp32-kbm/commit/e64d0b4d002c03ea44ecafcf1a7845cbc6e3727a)).

Currently, points 1, 2, 4, 6, 7 are developed.

## Break down of code for future references

### Security

- [ea78595](https://github.com/rampadc/esp32-kbm/commit/ea78595abd50857f87070e05ce31c7ff1f138acb): Built-in input capability, mitm, encryption, bonding from start
- [b6d86a0](https://github.com/rampadc/esp32-kbm/commit/b6d86a0bdd8a6683a8179699bfb723018646be9d): GATT permissions set to encrypted, was raising errors on Mac before when they were not encrypted
- [c56a5c3](https://github.com/rampadc/esp32-kbm/commit/c56a5c391f987cdc5c769a8ed6fd1a613fdd31b7): Send passkey to host using `p <passkey>` in UART, e.g.: if computer asks device to enter 842423, enter `p 842423` in the serial monitor.

### Console/Bluetooth functions

- [3d0b373](https://github.com/rampadc/esp32-kbm/commit/3d0b373fabdf45dc6bdb369c1e8d0bcc479aa881): First added console support
- [c56a5c3](https://github.com/rampadc/esp32-kbm/commit/c56a5c391f987cdc5c769a8ed6fd1a613fdd31b7): First passkey implementation. Used `passkey <passkey>` instead of `p <passkey>`.
- [e64d0b4](https://github.com/rampadc/esp32-kbm/commit/e64d0b4d002c03ea44ecafcf1a7845cbc6e3727a): Refactored to use queues to pass messages between bluetooth and console tasks
- [1eeb98e](https://github.com/rampadc/esp32-kbm/commit/1eeb98e4e6c9aa7c5e3fdceef9b3ee99bc52123a), [d4a662d](https://github.com/rampadc/esp32-kbm/commit/d4a662da4f777d6514820c5d0b62ea3535cbda9e): Send keycode with modifier to host. Use `r 0 31` for example, to send keycode 31 with modifier 0 to host. 
- [706ebf5](https://github.com/rampadc/esp32-kbm/commit/706ebf59c7e6e1516e924d26ffcef9c149bd1839): Added mouse command. **Unfixed BUG**: mouse commands cannot send negative number. `argtables` inteprets `m 0 -30 0` as -30 shortcode instead of -30 for x value.
- [54c0db2](https://github.com/rampadc/esp32-kbm/commit/54c0db2fb35496c5a175f7dd1c35a4f3119caaa8): List existing bonded devices
- [477b2a0](https://github.com/rampadc/esp32-kbm/commit/477b2a02c8bcd73cc1b2b686cd578461b08262cc): Function to get LED values from report. **BUG**: Not working
