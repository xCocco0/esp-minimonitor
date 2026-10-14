/**
 * @file
 *
 * @brief Touchpad emulation helpers
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_TOUCHPAD_H
#define __MM_TOUCHPAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum mm_touchpad_state_e {
    MM_TP_IDLE = 0,
    MM_TP_PRESSING,
    MM_TP_RIGHT_CLICKING,
    MM_TP_AFTER_TAP,
    MM_TP_DRAGGING,
    MM_TP_DOUBLE_TAP_1,
    MM_TP_DOUBLE_TAP_2,
    MM_TP_CURSOR,
};

typedef enum mm_touchpad_state_e mm_touchpad_state_t;

enum mm_touchpad_event_e {
    MM_TP_LEFT_CLICK_UP    = 1 << 0,
    MM_TP_LEFT_CLICK_DOWN  = 1 << 1,
    MM_TP_RIGHT_CLICK_UP   = 1 << 2,
    MM_TP_RIGHT_CLICK_DOWN = 1 << 3,
    MM_TP_TOUCH_UP         = 1 << 4,
    MM_TP_TOUCH_DOWN       = 1 << 5,
};

typedef enum mm_touchpad_event_e mm_touchpad_event_t;

struct mm_touchpad_data_s {
        mm_touchpad_state_t state;
        uint8_t pending_events;
        uint32_t t;
        int16_t touchdown_x,
                touchdown_y;
        int16_t last_x,
                last_y;
        int32_t ema_acc_dx,
                ema_acc_dy,
                last_err_dx,
                last_err_dy;
};

typedef struct mm_touchpad_data_s mm_touchpad_data_t;

struct mm_touchpad_s {
    uint32_t quick_tap_ms,
            long_tap_ms,
            double_tap_ms,
            key_time_ms;
    uint32_t dmax_px; /**< Maximum distance (in pixesl) between 2 taps to be considered a double tap. */
    uint16_t acceleration_gain; /**< How much the cursor speeds up when moving their finger quickly.
                                     A value of 0 disable acceleration. */
    uint16_t responsiveness; /**< This parameter makes the mouse movement smoother at the cost of latency.
                                     Should be between 1 and 100, where 100 disables the features and lower values
                                     increase the magnitude. */
    uint16_t sensitivity_x, /**< Multiplies the mouse output by the specified value. */
            sensitivity_y; /**< Multiplies the mouse output by the specified value. */
    mm_touchpad_data_t data;
};

typedef struct mm_touchpad_s mm_touchpad_t;

#define MM_TOUCHPAD_CONFIG_DEFAULT() (struct mm_touchpad_s) { \
    .quick_tap_ms = CONFIG_MM_TP_DEFAULT_QUICK_TAP_MS, \
    .long_tap_ms = CONFIG_MM_TP_DEFAULT_LONG_TAP_MS, \
    .double_tap_ms = CONFIG_MM_TP_DEFAULT_DOUBLE_TAP_MS, \
    .key_time_ms = CONFIG_MM_HID_DEFAULT_PRESS_DURATION_MS, \
    .dmax_px = CONFIG_MM_TP_DEFAULT_DMAX_PX, \
    .acceleration_gain = CONFIG_MM_TP_DEFAULT_ACCELERATION_GAIN, \
    .responsiveness = CONFIG_MM_TP_DEFAULT_RESPONSIVENESS, \
    .sensitivity_x = CONFIG_MM_TP_DEFAULT_X_SENSITIVITY, \
    .sensitivity_y = CONFIG_MM_TP_DEFAULT_Y_SENSITIVITY, \
}

void mm_touchpad_init(mm_touchpad_t *tp);
mm_touchpad_state_t mm_touchpad_get_state(mm_touchpad_t *tp);
mm_touchpad_event_t mm_touchpad_fsm_tick(mm_touchpad_t *tp, uint32_t delay_ms, int16_t x, int16_t y);
void mm_touchpad_fsm_reset(mm_touchpad_t *tp);
void mm_touchpad_send_event(mm_touchpad_t *tp, mm_touchpad_event_t event);
void mm_touchpad_touch_down(mm_touchpad_t *tp, int16_t x, int16_t y);
void mm_touchpad_touch_up(mm_touchpad_t *tp);

void mm_touchpad_mouse_delta_acceleration(mm_touchpad_t *tp, uint32_t dt, int16_t x, int16_t y, int32_t *dx_out, int32_t *dy_out);

#ifdef __cplusplus
}
#endif

#endif /* __MM_TOUCHPAD_H */
