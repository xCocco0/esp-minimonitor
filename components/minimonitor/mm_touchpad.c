/**
 * @file
 *
 * @brief Touchpad emulation helpers
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 *
 * Touchpad Timed Automaton description:
 * ```
 * [MM_TP_IDLE]
 * -> down? / t = 0, last_pos = pos() -> [MM_TP_PRESSING]
 * [MM_TP_PRESSING]
 * -> dist(pos(), last_pos) > DMAX -> [MM_TP_CURSOR]
 * -> up? && t < QUICK_TAP && dist(pos(), last_pos) < DMAX / t = 0, left_click_down! -> [MM_TP_AFTER_TAP]
 * -> t > LONG_TAP && dist(pos(), last_pos) < DMAX / t = 0, right_click_down! -> [MM_TP_RIGHT_CLICKING]
 * -> up? && QUICK_TAP < t < LONG_TAP -> [MM_TP_IDLE]
 * [MM_TP_RIGHT_CLICKING]
 * -> up? / right_click_up! -> [MM_TP_IDLE]
 * [MM_TP_AFTER_TAP]
 * -> down? && t < DRAG_DELAY / t = 0 -> [MM_TP_DRAGGING]
 * -> t > DRAG_DELAY / t = 0, left_click_up! -> [MM_TP_IDLE] // or move to idle and fire down again
 * [MM_TP_DRAGGING]
 * -> up? && t < QUICK_TAP / t = 0, left_click_up! -> [MM_TP_DOUBLE_TAP_1]
 * -> up? && t > QUICK_TAP / left_click_up! -> [MM_TP_IDLE]
 * [MM_TP_DOUBLE_TAP_1]
 * -> t > KEY_TIME / t = 0, left_click_down! -> [MM_TP_DOUBLE_TAP_2]
 * -> down? && t < KEY_TIME / t = 0, left_click_down! -> [MM_TP_DRAGGING]
 * [MM_TP_DOUBLE_TAP_2]
 * -> t > KEY_TIME / left_click_up! -> [MM_TP_IDLE]
 * -> down? && t < KEY_TIME / t = 0 -> [MM_TP_DRAGGING]
 * [MM_TP_CURSOR]
 * -> up? -> [MM_TP_IDLE]
 * ```
 */

#include "mm_touchpad.h"

#include "mm_macros.h"
#include "mm_log.h"
#include "mm_err.h"

#define __MOUSE_PRECISION_DIV (1 << 13)
#define __FP 256 /* multiplier for implementing fixed precision arithmetic */

#define __PRINT              0 /* print state change */
#define __PRINT_MOUSE_TRACE  0 /* print touch input to mouse output report */

#if __PRINT_MOUSE_TRACE

#define LOGSZ (512)
static int32_t *log = NULL;
#define __LOG(i,j) log[i*13+j]
static int log_i = 0;

#define __MOUSE_TRACE_ON_TOUCH_UP() do { \
    int last_zero_rows = 0; \
    for(int __i = 1; __i <= LOGSZ; ++__i) { \
        const int __ii = (log_i + __i) % LOGSZ; \
         \
        if(memcmp(&__LOG(__ii, 0), (int32_t[12]) {0}, 12*sizeof(int32_t)) == 0) { \
            last_zero_rows++; \
        } else { \
            if(last_zero_rows >= 2) { \
                MM_LOG_I("(...) skipped %d 0-rows", last_zero_rows); \
            } \
            last_zero_rows = 0; \
        } \
        if(last_zero_rows < 3) { \
            if(((__i - 1) % 30) == 0) { \
                MM_LOG_I("%6s   %-8.8s %-8.8s  %-8.8s %-8.8s  %-8.8s %-8.8s  %-8.8s %-8.8s  %-8.8s %-8.8s  %-8.8s %-8.8s", \
                        "dt", "dx", "dy", "dx_ema", "dy_ema", "speed_fp", "accelerara", "final_x", "final_y", "last_dx_er", "last_dy_er", "*dx_out", "*dy_out"); \
            } \
            MM_LOG_I("%6"PRId32": (%8"PRId32",%8"PRId32")(%8"PRId32",%8"PRId32")(%8"PRId32",%8"PRId32")" \
                    "(%8"PRId32",%8"PRId32")(%8"PRId32",%8"PRId32")(%8"PRId32",%8"PRId32")", \
                    __LOG(__ii,0), __LOG(__ii,1), __LOG(__ii, 2),__LOG(__ii,3), __LOG(__ii,4), __LOG(__ii,5), __LOG(__ii,6), \
                    __LOG(__ii,7), __LOG(__ii,8), __LOG(__ii,9), __LOG(__ii,10), __LOG(__ii,11), __LOG(__ii,13)); \
             \
        } \
    } \
    memset(log, '\0', sizeof(int32_t)*12*LOGSZ); \
} while(0)

#define __MOUSE_TRACE_ON_TOUCH_DOWN() do { \
        if(!log) { \
            log = calloc(12*LOGSZ, sizeof(int32_t)); \
            MM_CHECK_MALLOC_E(log); \
        } \
} while(0)

#define __MOUSE_TRACE_WRITE(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) do { \
    __LOG(log_i,  0) = (int32_t) v0; \
    __LOG(log_i,  1) = (int32_t) v1; \
    __LOG(log_i,  2) = (int32_t) v2; \
    __LOG(log_i,  3) = (int32_t) v3; \
    __LOG(log_i,  4) = (int32_t) v4; \
    __LOG(log_i,  5) = (int32_t) v5; \
    __LOG(log_i,  6) = (int32_t) v6; \
    __LOG(log_i,  7) = (int32_t) v7; \
    __LOG(log_i,  8) = (int32_t) v8; \
    __LOG(log_i,  9) = (int32_t) v9; \
    __LOG(log_i, 10) = (int32_t) v10; \
    __LOG(log_i, 11) = (int32_t) v11; \
    __LOG(log_i, 12) = (int32_t) v12; \
    log_i = (log_i + 1) % LOGSZ; \
} while(0)

#else
#define __MOUSE_TRACE_ON_TOUCH_UP()
#define __MOUSE_TRACE_ON_TOUCH_DOWN()
#define __MOUSE_TRACE_WRITE(...)
#endif

static inline void __right_click_down(mm_touchpad_t *tp)
{
    MM_LOG_I("Right click down");
    tp->data.pending_events |= MM_TP_RIGHT_CLICK_DOWN;
}

static inline void __right_click_up(mm_touchpad_t *tp)
{
    MM_LOG_I("Right click up");
    tp->data.pending_events |= MM_TP_RIGHT_CLICK_UP;
}

static inline void __left_click_down(mm_touchpad_t *tp)
{
    MM_LOG_I("Left click down");
    tp->data.pending_events |= MM_TP_LEFT_CLICK_DOWN;
}

static inline void __left_click_up(mm_touchpad_t *tp)
{
    MM_LOG_I("Left click up");
    tp->data.pending_events |= MM_TP_LEFT_CLICK_UP;
}

static inline void __clock_reset(mm_touchpad_t *tp)
{
    #if __PRINT
    MM_LOG_I("Clock %lu -> 0", tp->data.t);
    #endif
    tp->data.t = 0;
}

static int16_t __dist(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    int16_t dx, dy;
    dx = x1 > x2 ? x1 - x2 : x2 - x1;
    dy = y1 > y2 ? y1 - y2 : y2 - y1;
    return dx > dy ? dx + dy/2 : dy + dx/2;
}

static inline void __shift(mm_touchpad_t *tp, mm_touchpad_state_t new_state)
{
    #if __PRINT
    const char *s2n[] = {
        [MM_TP_IDLE]           = "IDLE",
        [MM_TP_PRESSING]       = "PRESSING",
        [MM_TP_RIGHT_CLICKING] = "RIGHT_CLICKING",
        [MM_TP_AFTER_TAP]      = "AFTER_TAP",
        [MM_TP_DRAGGING]       = "DRAGGING",
        [MM_TP_DOUBLE_TAP_1]   = "DOUBLE_TAP_1",
        [MM_TP_DOUBLE_TAP_2]   = "DOUBLE_TAP_2",
        [MM_TP_CURSOR]         = "CURSOR",
    };
    MM_LOG_I("%s -> %s\n", s2n[(int)tp->data.state], s2n[(int)new_state]);
    #endif
    tp->data.state = new_state;
}

void mm_touchpad_init(mm_touchpad_t *tp)
{
    if(tp->quick_tap_ms == 0) {
        tp->quick_tap_ms = 1;
    }
    if(tp->long_tap_ms == 0) {
        tp->long_tap_ms = 1;
    }
    if(tp->long_tap_ms <= tp->quick_tap_ms) {
        tp->long_tap_ms = tp->quick_tap_ms + 1;
    }
    if(tp->double_tap_ms == 0) {
        tp->double_tap_ms = 1;
    }
    if(tp->key_time_ms == 0) {
        tp->key_time_ms = 1;
    }
    if(tp->responsiveness <= 0 || tp->responsiveness > 100) {
        tp->responsiveness = 100;
    }
    if(tp->sensitivity_x == 0) {
        tp->sensitivity_x = 1;
    }
    if(tp->sensitivity_x > __MOUSE_PRECISION_DIV) {
        tp->sensitivity_x = __MOUSE_PRECISION_DIV;
    }
    if(tp->sensitivity_y == 0) {
        tp->sensitivity_y = 1;
    }
    if(tp->sensitivity_y > __MOUSE_PRECISION_DIV) {
        tp->sensitivity_y = __MOUSE_PRECISION_DIV;
    }

    mm_touchpad_fsm_reset(tp);
}

void mm_touchpad_fsm_reset(mm_touchpad_t *tp)
{
    tp->data.state = MM_TP_IDLE;
    tp->data.touchdown_x = 0;
    tp->data.touchdown_x = 0;
    tp->data.pending_events = 0;
    tp->data.t = 0;
}

void mm_touchpad_send_event(mm_touchpad_t *tp, mm_touchpad_event_t event)
{
    tp->data.pending_events |= event;
}

void mm_touchpad_touch_down(mm_touchpad_t *tp, int16_t x, int16_t y)
{
    tp->data.last_x = x;
    tp->data.last_y = y;
    tp->data.touchdown_x = x;
    tp->data.touchdown_y = y;
    tp->data.ema_acc_dx = 0;
    tp->data.ema_acc_dy = 0;
    tp->data.last_err_dx = 0;
    tp->data.last_err_dy = 0;

    mm_touchpad_send_event(tp, MM_TP_TOUCH_DOWN);
    __MOUSE_TRACE_ON_TOUCH_DOWN();
}

void mm_touchpad_touch_up(mm_touchpad_t *tp)
{
    mm_touchpad_send_event(tp, MM_TP_TOUCH_UP);
    __MOUSE_TRACE_ON_TOUCH_UP();
}

mm_touchpad_state_t mm_touchpad_get_state(mm_touchpad_t *tp)
{
    return tp->data.state;
}

mm_touchpad_event_t mm_touchpad_fsm_tick(mm_touchpad_t *tp, uint32_t delay_ms, int16_t x, int16_t y)
{
    uint32_t t;
    mm_touchpad_event_t output = 0;

    #define __up() (tp->data.pending_events & MM_TP_TOUCH_UP)
    #define __down() (tp->data.pending_events & MM_TP_TOUCH_DOWN)

    #define __take_up() (tp->data.pending_events &= ~MM_TP_TOUCH_UP)
    #define __take_down() (tp->data.pending_events &= ~MM_TP_TOUCH_DOWN)

    #define __save_last_pos(x, y) {tp->data.touchdown_x = x; tp->data.touchdown_y = y;}
    #define __dist_from_last_pos(x, y) (__dist(x, y, tp->data.touchdown_x, tp->data.touchdown_y))

    tp->data.t += delay_ms;
    t = tp->data.t;

    switch(tp->data.state) {
        case MM_TP_IDLE:
            if(__down()) {
                __take_down();
                t = 0;
                __save_last_pos(x, y);
                __shift(tp, MM_TP_PRESSING);
            }
            break;
        case MM_TP_PRESSING:
            if(__dist_from_last_pos(x,y) > tp->dmax_px) {
                __shift(tp, MM_TP_CURSOR);
            } else if(__up() && t < tp->quick_tap_ms) {
                __take_up();
                t = 0;
                __left_click_down(tp);
                __shift(tp, MM_TP_AFTER_TAP);
            } else if(t > tp->long_tap_ms) {
                t = 0;
                __right_click_down(tp);
                __shift(tp, MM_TP_RIGHT_CLICKING);
            } else if(__up()) {
                __take_up();
                __shift(tp, MM_TP_IDLE);
            }
            break;
        case MM_TP_RIGHT_CLICKING:
            if(__up()) {
                __take_up();
                __right_click_up(tp);
                __shift(tp, MM_TP_IDLE);
            }
            break;
        case MM_TP_AFTER_TAP:
            if(__down() && t < tp->double_tap_ms) {
                __take_down();
                t = 0;
                __shift(tp, MM_TP_DRAGGING);
            } else if(t > tp->double_tap_ms) {
                t = 0;
                __left_click_up(tp);
                __shift(tp, MM_TP_IDLE);
            }
            break;
        case MM_TP_DRAGGING:
            if(__up() && t < tp->quick_tap_ms) {
                __take_up();
                t = 0;
                __left_click_up(tp);
                __shift(tp, MM_TP_DOUBLE_TAP_1);
            } else if(__up() && t > tp->quick_tap_ms) {
                __take_up();
                __left_click_up(tp);
                __shift(tp, MM_TP_IDLE);
            }
            break;
        case MM_TP_DOUBLE_TAP_1:
            if(t > tp->key_time_ms) {
                t = 0;
                __left_click_down(tp);
                __shift(tp, MM_TP_DOUBLE_TAP_2);
            } else if(__down() && t < tp->key_time_ms) {
                __take_down();
                t = 0;
                __left_click_down(tp);
                __shift(tp, MM_TP_DRAGGING);
            }
            break;
        case MM_TP_DOUBLE_TAP_2:
            if(t > tp->key_time_ms) {
                __left_click_up(tp);
                __shift(tp, MM_TP_IDLE);
            } else if(__down() && t < tp->key_time_ms) {
                __take_down();
                t = 0;
                __shift(tp, MM_TP_DRAGGING);
            }
            break;
        case MM_TP_CURSOR:
            if(__up()) {
                __take_up();
                __shift(tp, MM_TP_IDLE);
            }
            break;
        default:
            break;
    }

    tp->data.t = t;
    output = tp->data.pending_events;
    tp->data.pending_events = 0;

    return output;
}

void mm_touchpad_mouse_delta_acceleration(mm_touchpad_t *tp, uint32_t dt, int16_t x, int16_t y, int32_t *dx_out, int32_t *dy_out)
{
    int16_t dx, dy;
    int32_t dx_ema, dy_ema;
    int32_t abs_x, abs_y;
    int32_t speed_fp;
    int32_t acceleration_gain_fp;
    int32_t final_x, final_y;

    dx = x - tp->data.last_x;
    dy = y - tp->data.last_y;
    dt = MAX(1, dt);

    tp->data.last_x = x;
    tp->data.last_y = y;

    /* EMA filter
       alpha = 0.25 (1/4) */

    dx_ema = (dx * (__FP * tp->responsiveness/100) + (tp->data.ema_acc_dx * (100 - tp->responsiveness) / 100));
    dy_ema = (dy * (__FP * tp->responsiveness/100) + (tp->data.ema_acc_dy * (100 - tp->responsiveness) / 100));

    tp->data.ema_acc_dx = dx_ema;
    tp->data.ema_acc_dy = dy_ema;

    /* compute speed in pixel/ms
       Alpha max plus beta min algorithm (alpha=1, beta=1/2):
       max(|x|, |y|) + 0.5 * min(|x|, |y|)
       note: the result is already scaled by FP_SCALE */

    abs_x = ABS(dx_ema);
    abs_y = ABS(dy_ema);
    speed_fp = (abs_x > abs_y ? abs_x + abs_y/2 : abs_y + abs_x/2) / dt;

    /* accelerate pointer based on speed */

    acceleration_gain_fp = __FP + (speed_fp * tp->acceleration_gain/100);

    final_x = MULT_AND_DIV(dx_ema, acceleration_gain_fp, __FP);
    final_y = MULT_AND_DIV(dy_ema, acceleration_gain_fp, __FP);

    /* scale based on the target sensitivity */

    *dx_out = (final_x + tp->data.last_err_dx) / (__MOUSE_PRECISION_DIV / tp->sensitivity_x);
    *dy_out = (final_y + tp->data.last_err_dy) / (__MOUSE_PRECISION_DIV / tp->sensitivity_y);

    tp->data.last_err_dx = (final_x + tp->data.last_err_dx) % (__MOUSE_PRECISION_DIV / tp->sensitivity_x);
    tp->data.last_err_dy = (final_y + tp->data.last_err_dy) % (__MOUSE_PRECISION_DIV / tp->sensitivity_y);

    __MOUSE_TRACE_WRITE(dt,dx,dy,dx_ema,dy_ema,speed_fp,acceleration_gain_fp,final_x,final_y,tp->data.last_err_dx,tp->data.last_err_dy,*dx_out,*dy_out);
}
