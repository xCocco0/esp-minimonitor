/**
 * @file
 *
 * @brief Minimonitor UI control pane
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui.h"

#include <lvgl.h>
#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_log.h"
#include "mm_event.h"
#include "mm_macros.h"
#include "mm_panic.h"
#include "mm_port.h"
#include "mm_sensor.h"
#include "mm_ui_screen.h"
#include "mm_ui_layout.h"
#include "mm_ui_state.h"
#include "mm_icon.h"
#include "mm_widgets.h"
#include "mm_ui_notification.h"
#include "mm_style.h"

static int s_initialized = 0;

static struct {
    int enabled;
    lv_timer_t *timeout_timer;
    struct mm_sensor_meter_config_s config;
} s_meter_data[MM_PORT_MAX_METERS];

static void __hide_meter_timeout_cb(lv_timer_t *timer);

static void __settime_handler_update_ui(mm_event_t id, void *data)
{
    struct timeval *tv = (struct timeval *) data;
    struct tm timeinfo;

    localtime_r(&tv->tv_sec, &timeinfo);

    if(tv->tv_sec > 0) {
        mm_ui_set_time(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, tv->tv_usec / 1000);
    }

    #if CONFIG_MM_UI_USE_CALENDAR
    lv_calendar_set_today_date(mm_widgets.calendar, 1900 + timeinfo.tm_year, 1 + timeinfo.tm_mon, timeinfo.tm_mday);
    lv_calendar_set_showed_date(mm_widgets.calendar, 1900 + timeinfo.tm_year, 1 + timeinfo.tm_mon);
    #endif

}

static void __notify_handler(mm_event_t id, void *data)
{
    struct mm_notify_param_s *param = (struct mm_notify_param_s *) data;
    mm_ui_notify(param);
}

static void __snooze_handler(mm_event_t id, void *data)
{
    char *name = (char *) data;
    if(name) {
        /* we are cancelling a specific notification */
        mm_ui_snooze(name);
    } else {
        /* we cancel all notifications */
        mm_ui_snooze(NULL);

        /* if we are in bsod, show normal screen */
        if(mm_screen_current == MM_SCREEN_PANIC) {
            MM_LOG_I("Leaving panic");
            mm_ui_screen_switch(MM_SCREEN_MAIN);
        }
    }
}

static void __setsensor_handler_update_meter(mm_event_t id, void *data)
{
    struct mm_sensor_data_s *param = (struct mm_sensor_data_s *) data;
    if(param->index >= MM_PORT_MAX_METERS) {
        MM_LOG_W("Index %d is beyond the maximum (%d)", param->index, MM_PORT_MAX_METERS);
        return;
    }
    mm_ui_set_meter(param->index, param->value1, param->value2, param->valueP);
}

static void __confsensor_handler(mm_event_t id, void *data)
{
    struct mm_sensor_meter_config_s *param = (struct mm_sensor_meter_config_s *) data;

    if(param->index >= MM_PORT_MAX_METERS) {
        MM_LOG_W("Index %d is beyond the maximum (%d)", param->index, MM_PORT_MAX_METERS);
        return;
    }
    if(strlen(param->label) > 0) {
        mm_ui_set_meter_config(param);
    } else {
        mm_ui_set_meter_disable(param->index);
    }
}

static void __change_tile_handler(mm_event_t id, void *data)
{
    mm_tile_command_t cmd = (mm_tile_command_t) data;
    static int swiping_right = 1;
    size_t cur_tile = mm_ui_state.current_tile_id;
    size_t next_tile;

    if(cur_tile == 0) {
        swiping_right = 1;
    }
    if(cur_tile == (MM_UI_N_TILES-1)) {
        swiping_right = 0;
    }

    if(cmd == MM_TILE_SWIPE_LEFT || (cmd == MM_TILE_CYCLE && !swiping_right)) {
        next_tile = cur_tile > 0 ? cur_tile-1 : 0;
    } else if(cmd == MM_TILE_SWIPE_RIGHT || (cmd == MM_TILE_CYCLE && swiping_right)) {
        next_tile = cur_tile < (MM_UI_N_TILES-1) ? cur_tile+1 : (MM_UI_N_TILES-1);
    } else if(cmd == MM_TILE_HOME) {
        next_tile = ((void *) &mm_tiles.by_name.tile_main - (void *) &mm_tiles)/sizeof(mm_tiles.by_id[0]);
    } else {
        return;
    }

    mm_ui_screen_switch_tile_id(next_tile);
}

static void __set_backlight_handler(mm_event_t id, void *data)
{
    mm_port_display_set_bk((uint8_t)(size_t) data);
}

static void __bsod_handler(mm_event_t id, void *data)
{
    struct mm_panic_info_s *param = (struct mm_panic_info_s *) data;
    char qr_link[80];
    char error_code[6];
    char *hostname = mm_conf_get_with_malloc(MM_CONF_HOSTNAME);

    mm_panic_info_to_code(param, error_code);
    lv_label_set_text_fmt(mm_widgets.lbl_panic_title, "Error (%s)", error_code);
    lv_label_set_text_fmt(mm_widgets.lbl_panic_subtitle, "Caused by: %s", param->crashed_task);

    if(hostname) {
        snprintf(qr_link, sizeof(qr_link), "http://%s.local/panic", hostname);
        lv_qrcode_update(mm_widgets.lbl_panic_qr, qr_link, strlen(qr_link));
        free(hostname);
    }

    mm_ui_screen_switch(MM_SCREEN_PANIC);
}

static void __tick_handler(mm_event_t id, void *data)
{
    uint32_t ret = mm_ui_tick();
    *((uint32_t *) data) = ret;
}

static void __motion_gesture_handler(mm_event_t id, void *data)
{
    const mm_motion_t mot = (mm_motion_t) data;
    mm_rotation_t rot;

    switch(mot) {
        case MM_MOTION_ANCHOR_LANDSCAPE:     rot = MM_ROT_0;   break;
        case MM_MOTION_ANCHOR_PORTRAIT:      rot = MM_ROT_90;  break;
        case MM_MOTION_ANCHOR_LANDSCAPE_INV: rot = MM_ROT_180; break;
        case MM_MOTION_ANCHOR_PORTRAIT_INV:  rot = MM_ROT_270; break;
        default: return;
    }

    #if CONFIG_MM_PORT_HAS_IMU
    if(!mm_ui_state.autorotation_enabled) {
        return;
    }
    #endif

    mm_ui_rotate_screen(rot);
}

static void __lv_log_puts(const char * buf)
{
    MM_LOG_I("%s", buf);
}

void mm_ui_init(void)
{
    MM_CHECK_E(s_initialized == 0);
    MM_CHECK_E(mm_conf_initialized());
    MM_CHECK_E(mm_port_initialized());
    MM_CHECK_E(mm_initialized());

    /* note: requires LV_USE_LOG and LV_LOG_LEVEL in Kconfig */
    lv_log_register_print_cb(__lv_log_puts);

    mm_style_init();

    mm_ui_screen_create_all();
    for(int i = 0; i < MM_PORT_MAX_METERS; ++i) {
        s_meter_data[i].timeout_timer = lv_timer_create(__hide_meter_timeout_cb, -1, (void *) i);
        lv_timer_set_repeat_count(s_meter_data[i].timeout_timer, -1);
        lv_timer_pause(s_meter_data[i].timeout_timer);
        s_meter_data[i].enabled = 0;
    }
    mm_ui_screen_switch(MM_SCREEN_MAIN);
    mm_layout_current = MM_LAYOUT_NOMETERS_SINGLE;
    mm_ui_layout_switch(mm_layout_current);

    mm_ui_state.dnd_enabled = mm_conf_get_as_integer(MM_CONF_DONOTDISTURB);
    mm_ui_state.autorotation_enabled = mm_conf_get_as_integer(MM_CONF_AUTOROTATION);

    mm_ui_tick();

    mm_event_add_handler(MM_EVENT_SETTIME, __settime_handler_update_ui);
    mm_event_add_handler(MM_EVENT_NOTIFY, __notify_handler);
    mm_event_add_handler(MM_EVENT_SNOOZE, __snooze_handler);
    mm_event_add_handler(MM_EVENT_SETSENSOR, __setsensor_handler_update_meter);
    mm_event_add_handler(MM_EVENT_CONFSENSOR, __confsensor_handler);
    mm_event_add_handler(MM_EVENT_CHANGETILE, __change_tile_handler);
    mm_event_add_handler(MM_EVENT_SETBACKLIGHT, __set_backlight_handler);
    mm_event_add_handler(MM_EVENT_BSOD, __bsod_handler);
    mm_event_add_handler(MM_EVENT_PERIODIC, __tick_handler);
    mm_event_add_handler(MM_EVENT_MOTION, __motion_gesture_handler);

    mm_ui_rotate_screen(mm_conf_get_as_integer(MM_CONF_PREFERRED_ROTATION));

    s_initialized = 1;
    MM_LOG_I("UI initialized");
}

int mm_ui_initialized(void)
{
    return s_initialized;
}

uint32_t mm_ui_tick(void)
{
    mm_layout_t new_layout;
    static int panel_important_prev = 0;

    new_layout = mm_ui_layout_from_current_state();

    if(new_layout != mm_layout_current) {
        mm_ui_layout_switch(new_layout);
        mm_layout_current = new_layout;
    }

    if(panel_important_prev != mm_ui_state.panel_important) {
        if(mm_ui_state.panel_important) {
            lv_obj_add_flag(mm_widgets.tray_notif->container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(mm_widgets.lbl_fps, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mm_widgets.lbl_fps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(mm_widgets.tray_notif->container, LV_OBJ_FLAG_HIDDEN);
        }
        panel_important_prev = mm_ui_state.panel_important;
    }

    return lv_timer_handler();
}

void mm_ui_notify(struct mm_notify_param_s *param)
{
    int opt;
    const char *name = param->name, *icon = param->icon, *msg = param->msg;

    if(!param->name) {
        name = "";
    }

    if(param->icon || param->name) {
        if(!icon) {
            icon = "bell";
        }
        if(!msg) {
            msg = "";
        }

        switch(param->opt) {
            case MM_NOTIFY_STACK:   opt = MM_NOTIF_STACK; break;
            case MM_NOTIFY_REPLACE: opt = MM_NOTIF_REPLACE; break;
            case MM_NOTIFY_APPEND:  opt = MM_NOTIF_APPEND; break;
            default:                opt = MM_NOTIF_DEFAULT; break;
        }
        if(param->persistent) {
            opt |= MM_NOTIF_PERSISTENT;
        }
        if(mm_ui_state.dnd_enabled) {
            opt |= MM_NOTIF_SILENT;
        }

        /* put notification into notif board and add an icon to notif tray */
        mm_ui_notif_post(name, icon, msg, opt);
        lv_obj_add_flag(mm_widgets.lbl_no_notif, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(mm_widgets.board_notif->lst, LV_OBJ_FLAG_HIDDEN);
    }

    if(param->duration_ms > 0 && param->color && !mm_ui_state.dnd_enabled) {
        /* glow animation on borders */
        mm_widget_glowframe_flash(mm_widgets.glowframe, param->duration_ms, param->color);
    }

    if(param->duration_ms > 0 && param->msg && !mm_ui_state.dnd_enabled) {
        /* show popup with message */
        mm_widget_toast_notify(mm_widgets.toast, param->duration_ms, msg);
    }
}

void mm_ui_toast(const char* msg, uint32_t duration_ms)
{
    if(duration_ms > 0 && msg) {
        mm_widget_toast_notify(mm_widgets.toast, duration_ms, msg);
    }
}

void mm_ui_snooze(const char *name)
{
    if(name) {
        mm_ui_notif_cancel_by_name(name);
    } else {
        mm_ui_notif_cancel_all();
        mm_widget_toast_cancel(mm_widgets.toast);
    }

    if(mm_ui_state.pending_notifications == 0) {
        lv_obj_clear_flag(mm_widgets.lbl_no_notif, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mm_widgets.board_notif->lst, LV_OBJ_FLAG_HIDDEN);
    }
}

void mm_ui_set_time(uint8_t h, uint8_t min, uint8_t sec, uint16_t ms)
{
    mm_widget_clock_settime(mm_widgets.clock, h, min, sec, ms);
}

void mm_ui_set_meter_config(struct mm_sensor_meter_config_s *config)
{
    s_meter_data[config->index].config = *config;

    lv_timer_set_period(s_meter_data[config->index].timeout_timer, config->timeout_ms);

    mm_widget_meterlist_meter_set_label(mm_widgets.meter_list, config->index, config->label);
    mm_widget_meterlist_meter_set_opt(mm_widgets.meter_list, config->index, config->type);
    mm_widget_meterlist_meter_hide(mm_widgets.meter_list, config->index);

    s_meter_data[config->index].enabled = 1;
}

void mm_ui_set_meter_enable(int index) {
    s_meter_data[index].enabled = 1;
}

void mm_ui_set_meter_disable(int index) {
    lv_timer_pause(s_meter_data[index].timeout_timer);
    mm_widget_meterlist_meter_hide(mm_widgets.meter_list, index);
    s_meter_data[index].enabled = 0;
}

static void __hide_meter_timeout_cb(lv_timer_t *timer)
{
    #if LVGL_VERSION_MAJOR >= 9
    int index = (int) lv_timer_get_user_data(timer);
    #else
    int index = (int) timer->user_data;
    #endif

    lv_timer_pause(timer);
    mm_widget_meterlist_meter_hide(mm_widgets.meter_list, index);
    mm_ui_state.panel_important = 0;
}

static uint32_t __mm_pow(uint32_t a, int b)
{
    uint32_t ret = 1;
    for(int i = 0; i < b; ++i) {
        ret *= a;
    }
    return ret;
}

void mm_ui_set_meter(int index, uint32_t value1, uint32_t value2, uint32_t valueP)
{
    char buf[CONFIG_MM_SENSOR_DESC_FMT_MAX_LEN + 20];
    uint32_t integer_part, decimal_part, color_v, wakeup_v;
    uint32_t color;
    const int n_decimals = s_meter_data[index].config.decimals;
    const struct mm_sensor_meter_config_s *conf = &s_meter_data[index].config;

    if(!s_meter_data[index].enabled) {
        return;
    }

    color_v = CHOOSE3(conf->color_arg, value1, value2, valueP);
    wakeup_v = CHOOSE3(conf->wakeup_arg, value1, value2, valueP);

    color = color_v < conf->threshold.low ? conf->color.base : (
            color_v < conf->threshold.high ? conf->color.warn :
            conf->color.alert);

    if(n_decimals > 0) {
        integer_part = value1 / __mm_pow(10, n_decimals);
        decimal_part = value1 % __mm_pow(10, n_decimals);
        snprintf(buf, sizeof(buf), conf->fmt, MM_SENSOR_PRINTF_ARGS(integer_part, decimal_part, n_decimals, value2));
    } else {
        snprintf(buf, sizeof(buf), conf->fmt, MM_SENSOR_PRINTF_ARGS(value1, 0, 0, value2));
    }

    mm_widget_meterlist_meter_set_color(mm_widgets.meter_list, index, color);
    mm_widget_meterlist_meter_set_value(mm_widgets.meter_list, index, (1000 * value1) / conf->max_value, buf);

    if(conf->timeout_ms == 0 || wakeup_v >= conf->wakeup_threshold) {
        mm_widget_meterlist_meter_show(mm_widgets.meter_list, index);
        if(conf->type & MM_METER_TYPE_PANEL) {
            if(valueP > conf->panel.wakeup_threshold) {
                mm_ui_state.panel_important = 1;
                snprintf(buf, sizeof(buf), conf->panel.fmt,
                        MM_SENSOR_PRINTF_ARGS(valueP, 0, 0, 0));
                lv_label_set_text(mm_widgets.lbl_fps, buf);
            } else {
                mm_ui_state.panel_important = 0;
            }
        }
    }

    if(conf->timeout_ms > 0 && wakeup_v >= conf->wakeup_threshold) {
        lv_timer_reset(s_meter_data[index].timeout_timer);
        lv_timer_resume(s_meter_data[index].timeout_timer);
    }
}

void mm_ui_rotate_screen(mm_rotation_t rot)
{
    lv_coord_t w, h, w_new, h_new, x, y;
    lv_obj_t *obj;

    #if CONFIG_MM_PORT_DISABLE_ROTATION_PORTRAIT
    if(rot == MM_ROT_90 || rot == MM_ROT_270) {
        return;
    }
    #endif

    if(mm_screen_current == MM_SCREEN_PANIC) {
        return;
    }

    mm_conf_set(MM_CONF_PREFERRED_ROTATION, &rot);
    mm_ui_state.current_rotation = rot;

    lv_obj_t *tv = mm_screen_root[MM_SCREEN_MAIN];

    w = lv_obj_get_width(tv);
    h = lv_obj_get_height(tv);

    MM_LOG_I("Setting screen rotation to %d", MM_ROTATION_TO_DEG(rot));

    mm_port_display_rotate(MM_ROTATION_TO_DEG(rot));

    w_new = lv_obj_get_width(tv);
    h_new = lv_obj_get_height(tv);

    if(w_new == w && h_new == h) {
        /* don't update tileview */
        return;
    }

    /* manually update tileview tiles to match new resolution */
    for(int i = 0; i < lv_obj_get_child_cnt(tv); i++) {
        obj = lv_obj_get_child(tv, i);
        x = lv_obj_get_x(obj);
        y = lv_obj_get_y(obj);
        lv_obj_set_pos(obj, x * w_new / w, y * h_new / h);
        //printf("%10p: (%4d, %4d) -> (%4d, %4d) \n", obj, x, y, x * w_new / w, y * h_new / h);
    }

    lv_obj_update_layout(tv);
    lv_obj_set_tile(tv, lv_tileview_get_tile_act(tv), LV_ANIM_OFF);
}
