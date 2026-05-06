/**
 * @file
 *
 * @brief Glowing overlay widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_glowframe.h"

#include "sdkconfig.h"
#include "mm_err.h"

void __glowframe_flash_once_cb(lv_timer_t * timer)
{
    #if LVGL_VERSION_MAJOR >= 9
    mm_widget_glowframe_t *w = (mm_widget_glowframe_t *) lv_timer_get_user_data(timer);
    #else
    mm_widget_glowframe_t *w = (mm_widget_glowframe_t *) timer->user_data;
    #endif

    /* first iteration: n_flashes even -> show frame
     * last iteration: n_flashes == 1 -> hide frame */
    if(w->n_flashes-- % 2) {
        lv_obj_add_flag(w->container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_set_style_border_color(w->container, lv_color_hex(w->color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(w->container, LV_OBJ_FLAG_HIDDEN);
    }

    if(w->n_flashes <= 0) {
        lv_timer_pause(timer);
    }
}

mm_widget_glowframe_t* mm_widget_glowframe_create(lv_obj_t* parent)
{
    struct mm_widget_glowframe_s *m;
    lv_obj_t *obj;

    m = (struct mm_widget_glowframe_s *) malloc(sizeof(struct mm_widget_glowframe_s));
    MM_CHECK_MALLOC_E(m);

    obj = lv_obj_create(parent);
    m->container = obj;
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

    m->n_flashes = 0;
    m->flash_timer = lv_timer_create(__glowframe_flash_once_cb, CONFIG_MM_NOTIF_FLASH_SINGLE_MS, NULL);
    lv_timer_set_repeat_count(m->flash_timer, -1);
    lv_timer_pause(m->flash_timer);
    #if LVGL_VERSION_MAJOR >= 9
    lv_timer_set_auto_delete(m->flash_timer, 0);
    #endif

    #if LVGL_VERSION_MAJOR >= 9
    lv_timer_set_user_data(m->flash_timer, (void *) m);
    #else
    m->flash_timer->user_data = (void *) m;
    #endif

    return m;
}

void mm_widget_glowframe_del(mm_widget_glowframe_t* widget)
{
    lv_timer_del(widget->flash_timer);
    lv_obj_del(widget->container);
    free(widget);
}

void mm_widget_glowframe_flash(mm_widget_glowframe_t* widget, unsigned long duration_ms, uint32_t color)
{
    int reps = (duration_ms / CONFIG_MM_NOTIF_FLASH_SINGLE_MS);
    /* reps must be even */
    if(reps % 2) reps++;
    widget->color = color;
    widget->n_flashes = reps;
    lv_timer_ready(widget->flash_timer);
    lv_timer_resume(widget->flash_timer);
}

void mm_widget_glowframe_cancel(mm_widget_glowframe_t* widget)
{
    lv_timer_pause(widget->flash_timer);
    widget->n_flashes = 1;
    __glowframe_flash_once_cb(widget->flash_timer);
}

