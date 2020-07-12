#ifndef BLE_KBM_TYPES_H
#define BLE_KBM_TYPES_H

#include <stdint.h>

typedef struct
{
    uint8_t modifier;
    uint8_t keycode;
} keyboard_t;

typedef struct
{
    uint8_t mouse_button;
    int8_t movement_x;
    int8_t moevement_y;
} mouse_t;

#endif