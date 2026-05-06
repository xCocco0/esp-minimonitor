/**
 * @file
 *
 * @brief Notification popup widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_toast.h"

#include "sdkconfig.h"
#include "mm_err.h"
#include "mm_font.h"
#include "mm_style.h"

void __toast_flash_once_cb(lv_timer_t * timer)
{
    #if LVGL_VERSION_MAJOR >= 9
    mm_widget_toast_t *w = (mm_widget_toast_t *) lv_timer_get_user_data(timer);
    #else
    mm_widget_toast_t *w = (mm_widget_toast_t *) timer->user_data;
    #endif

    lv_obj_add_flag(w->panel, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(timer);
}

mm_widget_toast_t* mm_widget_toast_create(lv_obj_t* parent)
{
    struct mm_widget_toast_s *m;
    lv_obj_t *obj;

    m = (struct mm_widget_toast_s *) malloc(sizeof(struct mm_widget_toast_s));
    MM_CHECK_MALLOC_E(m);

    obj = lv_obj_create(parent);
    m->panel = obj;
    mm_add_style_panel(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* lbl_toast */
    obj = lv_label_create(m->panel);
    m->lbl = obj;
    mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(obj, 0, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT); // if multi-line: panel taller
    //lv_obj_set_size(obj, LV_PCT(100), MM_FONT_SMALL->line_height);
    lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "");

    m->timer = lv_timer_create(__toast_flash_once_cb, -1, (void *) m);
    lv_timer_pause(m->timer);
    lv_timer_set_repeat_count(m->timer, -1);
    #if LVGL_VERSION_MAJOR >= 9
    lv_timer_set_auto_delete(m->timer, 0);
    #endif

    return m;
}

void mm_widget_toast_del(mm_widget_toast_t* widget)
{
    lv_timer_del(widget->timer);
    lv_obj_del(widget->panel);
    free(widget);
}

static void __move_y_cb(void *var, int32_t v)
{
    lv_obj_set_y(var, v);
}

void mm_widget_toast_notify(mm_widget_toast_t* widget, unsigned long duration_ms, const char *msg)
{
    lv_label_set_text(widget->lbl, msg);
    lv_obj_clear_flag(widget->panel, LV_OBJ_FLAG_HIDDEN);

    #if CONFIG_MM_UI_ANIMATED
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, widget->panel);
    lv_anim_set_values(&a, MM_PORT_BORDER_BOTTOM + lv_obj_get_height(widget->panel), 0);
    lv_anim_set_time(&a, 150 * CONFIG_MM_UI_ANIM_SPEED / 100);
    lv_anim_set_exec_cb(&a, __move_y_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    #endif

    lv_timer_set_period(widget->timer, duration_ms);
    lv_timer_reset(widget->timer);
    lv_timer_resume(widget->timer);
}

void mm_widget_toast_cancel(mm_widget_toast_t* widget)
{
    lv_timer_pause(widget->timer);
    __toast_flash_once_cb(widget->timer);
}

