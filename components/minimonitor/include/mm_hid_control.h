/**
 * @file
 *
 * @brief Helpers for the HID control UI
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_HID_CONTROL_H
#define __MM_HID_CONTROL_H

#include <stdint.h>
#include "mm_err.h"
#include "mm_hid_touchpad.h"

#define MM_KEY_IS_KEYBOARD(key)  (MM_KEY_KEYBOARD_START < (key) && (key) < MM_KEY_KEYBOARD_END)
#define MM_KEY_IS_MODIFIER(key)  (MM_KEY_MODIFIERS_START < (key) && (key) < MM_KEY_MODIFIERS_END)
#define MM_KEY_IS_MOUSE(key)     (MM_KEY_MOUSE_START < (key) && (key) < MM_KEY_MOUSE_END)
#define MM_KEY_IS_CONSUMER(key)  (MM_KEY_CONSUMER_START < (key) && (key) < MM_KEY_CONSUMER_END)
#define MM_KEY_IS_VALID(key)     (MM_KEY_IS_KEYBOARD(key) || MM_KEY_IS_MODIFIER(key) || MM_KEY_IS_MOUSE(key) || MM_KEY_IS_CONSUMER(key))

#define MM_KEY_IS_MOUSE_CURSOR(key) ( (key) == MM_KEY_MOUSE_ABS_X \
                                    || (key) == MM_KEY_MOUSE_ABS_Y \
                                    || (key) == MM_KEY_MOUSE_DELTA_X \
                                    || (key) == MM_KEY_MOUSE_DELTA_Y )

#define MM_KEY_TO_STR(key) (mm_key2str_map[(key)])
#define MM_KEY_TO_STR_LEN(key) ((size_t) mm_key2strlen_map[(key)])
#define MM_KEY_TO_LONG_STR(key) (mm_key2longstr_map[(key)])
#define MM_KEY_TO_VALUE(key) (mm_key2value_map[(key)])

#ifdef __cplusplus
extern "C"
{
#endif

enum mm_key_e {

    MM_KEY_NONE = 0,

    #define FOR_EACH_KEY(MACRONAME, value, fancy_name, short_name) MM_##MACRONAME ,

    /* Keyboard keys (without modifiers) */
    MM_KEY_KEYBOARD_START,
    #include "mm_keys_keyboard.def"
    MM_KEY_KEYBOARD_END,

    /* Modifier keys */
    MM_KEY_MODIFIERS_START,
    #include "mm_keys_modifiers.def"
    MM_KEY_MODIFIERS_END,

    /* Mouse keys/controls */
    MM_KEY_MOUSE_START,
    #include "mm_keys_mouse.def"
    MM_KEY_MOUSE_END,

    /* Consumer controls */
    MM_KEY_CONSUMER_START,
    #include "mm_keys_consumer.def"
    MM_KEY_CONSUMER_END,

    #undef FOR_EACH_KEY

    MM_KEY_ENUM_MAX,

} __attribute__((packed));

typedef enum mm_key_e mm_key_t;

extern const char *mm_key2str_map[MM_KEY_ENUM_MAX];
extern const uint8_t mm_key2strlen_map[MM_KEY_ENUM_MAX];
extern const char *mm_key2longstr_map[MM_KEY_ENUM_MAX];
extern const uint8_t mm_key2value_map[MM_KEY_ENUM_MAX];

struct mm_hid_action_s {
    mm_key_t key;
    uint8_t key_down;
    int16_t magnitude;
    struct {
        uint32_t times;
        uint32_t delay;
        uint32_t jitter;
    } repeat;
};

typedef enum {
    MM_HID_CONTROL_INVALID = 0,
    MM_HID_CONTROL_AUTOCLICK,
    MM_HID_CONTROL_MATRIX,
    MM_HID_CONTROL_TOUCHPAD,
} mm_hid_control_type_t;

typedef enum {
    MM_HID_PRESS_ONCE = 0, /**< On press run macro once as is. */
    MM_HID_PRESS_TOGGLE, /**< Press once to trigger key down, then press once again to trigger key up. */
    MM_HID_PRESS_HOLD, /**< Start pressing triggers key down and release triggers key up. */
} mm_hid_press_behaviour_t;

struct mm_hid_control_config_s {
    size_t index;
    mm_hid_control_type_t type;
    union {
        struct {
            uint32_t period_ms; /**< Only meaningful when `press_bhv != MM_HID_PRESS_ONCE` */
            uint32_t jitter_ms; /**< Absolute maximum deviation from period_ms */
            mm_hid_press_behaviour_t press_bhv;
            char *icon;
            char *macro;
        } autoclicker;
        struct {
            size_t n_buttons;
            char **icon_v;
            char **macro_v;
        } matrix;
        struct {
            int16_t sensitivity_x; /**< Output is multiplied by this number and divided by 1000 (default = 20) */
            int16_t sensitivity_y; /**< Output is multiplied by this number and divided by 1000 (default = 20) */
        } touchpad;
    } u;
};

#define MM_HID_CONTROL_CONFIG_DEFAULT_AUTOCLICK(x) (struct mm_hid_control_config_s) { \
    .index = 0, \
    .type = MM_HID_CONTROL_AUTOCLICK, \
    .u.autoclicker = { \
        .icon = "", \
        .period_ms = 0, \
        .jitter_ms = 0, \
        .press_bhv = MM_HID_PRESS_ONCE \
    } \
}
#define MM_HID_CONTROL_CONFIG_DEFAULT_MATRIX(x) (struct mm_hid_control_config_s) { \
    .index = 0, \
    .type = MM_HID_CONTROL_MATRIX, \
    .u.matrix = { \
        .n_buttons = 0, \
    } \
}
#define MM_HID_CONTROL_CONFIG_DEFAULT_TOUCHPAD() (struct mm_hid_control_config_s) { \
    .index = 0, \
    .type = MM_HID_CONTROL_TOUCHPAD, \
    .u.touchpad = { \
        .sensitivity_x = CONFIG_MM_TP_DEFAULT_X_SENSITIVITY, \
        .sensitivity_y = CONFIG_MM_TP_DEFAULT_Y_SENSITIVITY, \
    } \
}

mm_key_t mm_hid_control_find_key_prefix(const char *key_name);

mm_err_t mm_hid_control_config_update(struct mm_hid_control_config_s *config);
struct mm_hid_control_config_s *mm_hid_control_config_fetch(int index);
void mm_hid_control_config_destroy(struct mm_hid_control_config_s *config);
mm_err_t mm_hid_control_config_erase(int index);

struct mm_hid_control_config_s *mm_hid_control_config_deepcopy(struct mm_hid_control_config_s * config);
void mm_hid_control_config_deepfree(struct mm_hid_control_config_s *config);

#ifdef __cplusplus
}
#endif

#endif /* __MM_HID_CONTROL_H */
