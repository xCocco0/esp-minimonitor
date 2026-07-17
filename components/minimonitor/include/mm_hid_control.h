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
#include "mm_touchpad.h"

#define MM_KEY_IS_KEYBOARD(key)  (MM_KEY_KEYBOARD_START < (key) && (key) < MM_KEY_KEYBOARD_END)
#define MM_KEY_IS_MODIFIER(key)  (MM_KEY_MODIFIERS_START < (key) && (key) < MM_KEY_MODIFIERS_END)
#define MM_KEY_IS_MOUSE(key)     (MM_KEY_MOUSE_START < (key) && (key) < MM_KEY_MOUSE_END)
#define MM_KEY_IS_CONSUMER(key)  (MM_KEY_CONSUMER_START < (key) && (key) < MM_KEY_CONSUMER_END)
#define MM_KEY_IS_VALID(key)     (MM_KEY_IS_KEYBOARD(key) || MM_KEY_IS_MODIFIER(key) || MM_KEY_IS_MOUSE(key) || MM_KEY_IS_CONSUMER(key))

#define MM_KEY_TO_STR(key) (mm_key2str_map[(key)])
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
extern const char *mm_key2longstr_map[MM_KEY_ENUM_MAX];
extern const uint8_t mm_key2value_map[MM_KEY_ENUM_MAX];

enum mm_hid_action_type_e {

    MM_HID_ACTION_RELEASE = 0,
    MM_HID_ACTION_MOUSE,
    MM_HID_ACTION_KEYBOARD,
    MM_HID_ACTION_CONSUMER,

    MM_HID_ACTIONS_N,
};

typedef enum mm_hid_action_type_e mm_hid_action_type_t;

struct mm_hid_action_s {
    mm_hid_action_type_t type;
    union {
        struct {
            uint8_t buttons;
            int16_t x, /*<< range [-127,127] (or [0, 32767] if absolute), positive: right, negative: left */
            y; /*<< range [-127,127] (or [0, 32767] if absolute), positive: down, negative: up */
            int is_absolute;
            int8_t wheel; /*<< range [-127,127] */
            int8_t pad; /*<< range [-127,127] */
        } mouse_report;
        struct {
            uint8_t modifiers;
            uint8_t keycode[6];
        } keyboard_report;
        struct {
            uint16_t control;
        } consumer_report;
    } u;
};

typedef enum {
    MM_HID_CONTROL_INVALID = 0,
    MM_HID_CONTROL_AUTOCLICK,
    MM_HID_CONTROL_MATRIX,
    MM_HID_CONTROL_TOUCHPAD,
} mm_hid_control_type_t;

struct mm_hid_control_config_s {
    int index;
	mm_hid_control_type_t type;
	union {
		struct {
            mm_key_t keys[CONFIG_MM_HID_MAX_NKRO];
            char icon[CONFIG_MM_MAX_ICON_NAME_LEN+1];
            uint32_t duration_ms; /**< duration == 0: toggle click */
            uint32_t period_ms; /**< period_ms == 0: single click (unless duration_ms is also 0) */
            int16_t intensity;
            uint16_t jitter_ms; /**< absolute maximum deviation from period_ms */
		} autoclicker;
		struct {
			mm_key_t keys[CONFIG_MM_HID_CONTROL_MATRIX_MAX_BTN][CONFIG_MM_HID_MAX_NKRO];
            char icons[CONFIG_MM_HID_CONTROL_MATRIX_MAX_BTN][CONFIG_MM_MAX_ICON_NAME_LEN+1];
            int16_t intensity;
		} matrix;
        struct {
            int16_t sensitivity_x; /**< output is multiplied by this number and divided by 1000 (default = 20) */
            int16_t sensitivity_y; /**< output is multiplied by this number and divided by 1000 (default = 20) */
        } touchpad;
	} u;
};

#define MM_HID_CONTROL_CONFIG_DEFAULT_AUTOCLICK(x) (struct mm_hid_control_config_s) { \
    .index = 0, \
    .type = MM_HID_CONTROL_AUTOCLICK, \
    .u.autoclicker = { \
        .icon = "", \
        .duration_ms = 20, \
        .period_ms = 0, \
        .intensity = 1, \
        .jitter_ms = 0 \
    } \
}
#define MM_HID_CONTROL_CONFIG_DEFAULT_MATRIX(x) (struct mm_hid_control_config_s) { \
    .index = 0, \
    .type = MM_HID_CONTROL_MATRIX, \
    .u.matrix = { \
        .intensity = 0 \
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

mm_err_t mm_hid_control_keycode_to_fullname(char *buf, size_t bufsize, mm_key_t *keys, size_t nkeys);
int mm_hid_control_fullname_to_key_arr(mm_key_t *keys, size_t keys_bufsize, const char *str);

mm_err_t mm_hid_control_config_to_action(struct mm_hid_action_s *act_m, struct mm_hid_action_s *act_kb, struct mm_hid_action_s *act_c, struct mm_hid_control_config_s *conf);
mm_err_t mm_hid_control_key_to_action(struct mm_hid_action_s *act, mm_key_t key, int16_t intensity);
mm_err_t mm_hid_control_key_arr_to_action(struct mm_hid_action_s *act_m, struct mm_hid_action_s *act_kb, struct mm_hid_action_s *act_c, mm_key_t *keys, size_t n_keys, int16_t intensity);

mm_err_t mm_hid_control_store_config(struct mm_hid_control_config_s *config);
mm_err_t mm_hid_control_fetch_config(struct mm_hid_control_config_s *config);
mm_err_t mm_hid_control_erase_config(int index);

#ifdef __cplusplus
}
#endif

#endif /* __MM_HID_CONTROL_H */
