/**
 * @file
 *
 * @brief Touchpad screen
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_screen.h"
#include "sdkconfig.h"

#include <stdint.h>
#include <time.h>
#include "mm_conf.h"
#include "mm_macros.h"
#include "mm_err.h"
#include "mm_event.h"
#include "mm_hid_control.h"
#include "mm_port_ui_customization.h"
#include "mm_style.h"
#include "mm_font.h"
#include "mm_widgets.h"
#include "mm_touchpad.h"

#define MOUSE_PANEL_HEIGHT LV_PCT(20)

#define __TIMESPEC_TO_MS(t) ((uint32_t) ((t)->tv_nsec/1000000 + (t)->tv_sec*1000))
#define __TIMESPEC_TO_US(t) ((uint32_t) ((t)->tv_nsec/1000 + (t)->tv_sec*1000000))

#if CONFIG_MM_IO_ENABLE_USB_HID

static lv_indev_t *s_indev = NULL;
static int s_fsm_running = 0;
static struct mm_hid_action_s a;
static mm_touchpad_t s_touchpad;

static inline void __timespec_diff(struct timespec *result, const struct timespec *a, const struct timespec *b)
{
    if((a->tv_nsec - b->tv_nsec) < 0) {
        result->tv_sec = a->tv_sec - b->tv_sec - 1;
        result->tv_nsec = a->tv_nsec - b->tv_nsec + 1000000000L;
    } else {
        result->tv_sec = a->tv_sec - b->tv_sec;
        result->tv_nsec = a->tv_nsec - b->tv_nsec;
    }
}

static void __screen_loaded(lv_event_t *e)
{
    const struct mm_hid_control_config_s *conf = mm_widget_hidaction_last_touchpad_config();
    s_touchpad.sensitivity_x = conf->u.touchpad.sensitivity_x;
    s_touchpad.sensitivity_y = conf->u.touchpad.sensitivity_y;
    mm_touchpad_init(&s_touchpad);
}

static void __background_pressing_handler(mm_event_t id, void *data)
{
    lv_point_t point;
    int32_t x, y;
    struct timespec now, dt;
    static struct timespec last_triggered = {0,0};
    mm_touchpad_event_t out;

    clock_gettime(CLOCK_MONOTONIC, &now);
    lv_indev_get_point(s_indev, &point);
    __timespec_diff(&dt, &now, &last_triggered);

    /* determine key press/release */

    out = mm_touchpad_fsm_tick(&s_touchpad, __TIMESPEC_TO_MS(&dt), point.x, point.y);

    if(out & MM_TP_LEFT_CLICK_DOWN) {
        a.u.mouse_report.buttons |= MM_KEY_TO_VALUE(MM_KEY_MOUSE_LEFT);
    }
    if(out & MM_TP_RIGHT_CLICK_DOWN) {
        a.u.mouse_report.buttons |= MM_KEY_TO_VALUE(MM_KEY_MOUSE_RIGHT);
    }
    if(out & MM_TP_LEFT_CLICK_UP) {
        a.u.mouse_report.buttons &= ~MM_KEY_TO_VALUE(MM_KEY_MOUSE_LEFT);
    }
    if(out & MM_TP_RIGHT_CLICK_UP) {
        a.u.mouse_report.buttons &= ~MM_KEY_TO_VALUE(MM_KEY_MOUSE_RIGHT);
    }

    /* compute mouse pointer shift */

    mm_touchpad_mouse_delta_acceleration(&s_touchpad, __TIMESPEC_TO_MS(&dt), point.x, point.y, &x, &y);

    a.type = MM_HID_ACTION_MOUSE;
    a.u.mouse_report.x = 0;
    a.u.mouse_report.y = 0;
    do {
        if(x < -127) {
            a.u.mouse_report.x = -127;
            x += 127;
        } else if(x > 127) {
            a.u.mouse_report.x = 127;
            x -= 127;
        } else {
            a.u.mouse_report.x = x;
            x = 0;
        }
        if(y < -127) {
            a.u.mouse_report.y = -127;
            y += 127;
        } else if(y > 127) {
            a.u.mouse_report.y = 127;
            y -= 127;
        } else {
            a.u.mouse_report.y = y;
            y = 0;
        }

        mm_event_queue_sendhid(&a);
    } while(x || y);

    if(mm_touchpad_get_state(&s_touchpad) == MM_TP_IDLE) {
        mm_event_delete_handler(MM_EVENT_BACKGROUND, __background_pressing_handler);
        /* restore indev polling rate */
        lv_timer_set_period(s_indev->driver->read_timer, CONFIG_LV_DISP_DEF_REFR_PERIOD);
        s_fsm_running = 0;
    }
    last_triggered = now;
}

static void __screen_start_touching(lv_event_t *e)
{
    lv_point_t point;
    s_indev = lv_event_get_indev(e);
    lv_indev_get_point(s_indev, &point);
    mm_touchpad_touch_down(&s_touchpad, point.x, point.y);
    /* note: we don't use the LV_EVENT_PRESSING because it is too
       slow and would make the mouse cursor lag */
    if(!s_fsm_running) {
        memset(&a, 0, sizeof(struct mm_hid_action_s));
        mm_event_add_handler(MM_EVENT_BACKGROUND, __background_pressing_handler);
        /* make the indev faster to reduce lag */
        lv_timer_set_period(s_indev->driver->read_timer, CONFIG_MM_EVENT_TASK_PERIOD_MS);
        s_fsm_running = 1;
    }
}

static void __screen_stop_touching(lv_event_t *e)
{
    mm_touchpad_touch_up(&s_touchpad);
}

static void __quit_pressed(lv_event_t *e)
{
    if(mm_touchpad_get_state(&s_touchpad) == MM_TP_IDLE) {
        MM_LOG_I("Closing touchpad");
        mm_event_delete_handler(MM_EVENT_BACKGROUND, __background_pressing_handler);
        s_fsm_running = 0;
        mm_ui_screen_switch(MM_SCREEN_MAIN);
        mm_ui_screen_switch_tile(&mm_tiles.by_name.tile_hid, 0);
    }
}

void mm_ui_screen_create_touchpad(void)
{
        lv_obj_t *screen_touchpad;
        lv_obj_t *btn_quit;
        lv_obj_t *lbl_quit;
        lv_obj_t *img_mouse;

        /* main screen */
        screen_touchpad = lv_obj_create(0);
        mm_screen_root[MM_SCREEN_TOUCHPAD] = screen_touchpad;
        mm_add_style_screen(screen_touchpad, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_pad_left(screen_touchpad, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(screen_touchpad, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(screen_touchpad, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(screen_touchpad, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(screen_touchpad, mm_style_color_card(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(screen_touchpad, LV_SCROLLBAR_MODE_OFF);

        lv_obj_add_event_cb(screen_touchpad, __screen_start_touching, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(screen_touchpad, __screen_stop_touching, LV_EVENT_RELEASED, NULL);
        lv_obj_add_event_cb(screen_touchpad, __screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);

        s_touchpad = MM_TOUCHPAD_CONFIG_DEFAULT();
        mm_touchpad_init(&s_touchpad);

        img_mouse = lv_img_create(screen_touchpad);
        lv_obj_center(img_mouse);
        lv_img_set_src(img_mouse, MM_IMG_SRC_TAP);
        lv_img_set_zoom(img_mouse, 256*2);
        lv_img_set_antialias(img_mouse, 0);
        lv_obj_set_style_img_recolor(img_mouse, mm_style_color_detail(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_img_recolor_opa(img_mouse, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        btn_quit = lv_btn_create(screen_touchpad);
        mm_add_style_panel(btn_quit, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(btn_quit, MM_PORT_FONT_SIZE_TINY + MM_PORT_BORDER_INNER, MM_PORT_FONT_SIZE_TINY + MM_PORT_BORDER_INNER);
        lv_obj_align(btn_quit, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_scrollbar_mode(btn_quit, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(btn_quit, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(btn_quit, __quit_pressed, LV_EVENT_PRESSED, (void *) btn_quit);

        lbl_quit = lv_label_create(btn_quit);
        lv_obj_center(lbl_quit);
        lv_obj_set_size(lbl_quit, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(lbl_quit, MM_FONT_TINY, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text_static(lbl_quit, "X");
}

#else
void mm_ui_screen_create_touchpad(void) {}
#endif
