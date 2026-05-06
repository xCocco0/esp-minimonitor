/**
 * @file
 *
 * @brief Clock widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_clock.h"

#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_conf.h"
#include "mm_err.h"
#include "mm_font.h"
#include "mm_widgets.h"
#include "mm_style.h"

#define __24H_TO_12H(h) (((h + 11) % 12) + 1)

static inline void __increase_minutes(mm_widget_clock_t *w)
{
    if(++(w->minutes) >= 60) {
        w->minutes = 0;
        if(++(w->hours) >= 24) {
            w->hours = 0;
        }
    }
}

static inline void __increase_seconds(mm_widget_clock_t *w)
{
    if(++(w->seconds) >= 60) {
        w->seconds = 0;
        __increase_minutes(w);
    }
}

static inline void __update_time(mm_widget_clock_t *w)
{
    if(mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
        /* update seconds*/
        sprintf(w->buf_seconds, "%02hu", w->seconds);
        lv_label_set_text_static(w->lbl_seconds, w->buf_seconds);
        lv_obj_invalidate(w->lbl_seconds);
    }

    /* update minutes */
    sprintf(w->buf_minutes, "%02hu", w->minutes);
    lv_label_set_text_static(w->lbl_minutes, w->buf_minutes);
    lv_obj_invalidate(w->lbl_minutes);

    /* update seconds */
    if(mm_conf_get_as_integer(MM_CONF_CLOCK_FORMAT_HOUR) == 12) {
        sprintf(w->buf_hours, "%hu", __24H_TO_12H(w->hours));
    } else {
        sprintf(w->buf_hours, "%hu", w->hours);
    }
    lv_label_set_text_static(w->lbl_hours, w->buf_hours);
    lv_obj_invalidate(w->lbl_hours);
}

void __refresh_time_cb(lv_timer_t *timer)
{
    #if LVGL_VERSION_MAJOR >= 9
    mm_widget_clock_t *w = (mm_widget_clock_t *) lv_timer_get_user_data(timer);
    #else
    mm_widget_clock_t *w = (mm_widget_clock_t *) timer->user_data;
    #endif

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_SEPARATOR_ANIMATED)) {

        if(w->separator_shown) {
            lv_obj_set_style_text_opa(w->lbl_colon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            if(mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
                lv_obj_set_style_text_opa(w->lbl_colon2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            w->separator_shown = 0;
        } else {
            lv_obj_set_style_text_opa(w->lbl_colon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            if(mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
                lv_obj_set_style_text_opa(w->lbl_colon2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            w->separator_shown = 1;
        }
    }

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_SEPARATOR_ANIMATED) \
            || mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
        __increase_seconds(w);
    } else {
        __increase_minutes(w);
    }

    __update_time(w);
}

mm_widget_clock_t *mm_widget_clock_create(lv_obj_t *parent)
{
    struct mm_widget_clock_s *m;
    lv_obj_t *obj;
    char buf[8];
    size_t bufsize = sizeof(buf);

    m = (struct mm_widget_clock_s *) malloc(sizeof(struct mm_widget_clock_s));
    MM_CHECK_MALLOC_E(m);

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_SEPARATOR_ANIMATED)) {
        m->separator_shown = 1;
    }

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_SEPARATOR_ANIMATED) \
            || mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
        /* fire every second */
        m->refresh_time_timer = lv_timer_create(__refresh_time_cb, 1000, (void *) m);
    } else {
        /* fire every minute */
        m->refresh_time_timer = lv_timer_create(__refresh_time_cb, 60*1000, (void *) m);
    }
    lv_timer_set_repeat_count(m->refresh_time_timer, -1);
    lv_timer_resume(m->refresh_time_timer);

    /* main container */
    obj = lv_obj_create(parent);
    m->container = obj;
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    MM_STYLE_EMPTY_CONT(obj);
    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* hours label */
    obj = lv_label_create(m->container);
    m->lbl_hours = obj;
    mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(obj, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "--");

    obj = lv_label_create(m->container);
    m->lbl_colon = obj;
    mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(obj, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    if(mm_conf_get(MM_CONF_CLOCK_H_MIN_SEPARATOR, buf, &bufsize) != MM_OK) {
        strcpy(buf, ":");
    }
    lv_label_set_text(obj, buf);

    obj = lv_label_create(m->container);
    m->lbl_minutes = obj;
    mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(obj, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "--");

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {

        obj = lv_label_create(m->container);
        m->lbl_colon2 = obj;
        mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(obj, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, buf);

        obj = lv_label_create(m->container);
        m->lbl_seconds = obj;
        mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(obj, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "--");

    }

    return m;
}

void mm_widget_clock_del(mm_widget_clock_t *widget)
{
    lv_obj_del(widget->container);
    lv_timer_del(widget->refresh_time_timer);
    free(widget);
}

static void __timer_set_time_remaining(lv_timer_t *timer, uint32_t period, uint32_t ms)
{
    /* we need to set:
     * timer->last_run = lv_tick_get() - (period - ms);
     *
     * we exploit the fact that:
     * lv_timer_ready(timer) := timer->last_run = lv_tick_get() - (period + 1);
     */
    lv_timer_set_period(timer, period - ms - 1);
    lv_timer_ready(timer);
    lv_timer_set_period(timer, period);
}

void mm_widget_clock_settime(mm_widget_clock_t *widget, uint8_t h, uint8_t min, uint8_t sec, uint16_t ms)
{
    widget->hours = h;
    widget->minutes = min;
    widget->seconds = sec;

    __update_time(widget);

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_SEPARATOR_ANIMATED) \
            || mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
        const uint16_t ms_until_next_sec = 1000 - ms;
        __timer_set_time_remaining(widget->refresh_time_timer, 1000, ms_until_next_sec);
    } else {
        const uint16_t ms_until_next_min = 60*1000 - sec*1000 - ms;
        __timer_set_time_remaining(widget->refresh_time_timer, 60*1000, ms_until_next_min);
    }
}
