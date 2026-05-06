/**
 * @file
 *
 * @brief Meter widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_meter.h"

#include "sdkconfig.h"
#include "mm_err.h"
#include "mm_port_ui_customization.h"
#include "mm_font.h"
#include "mm_widgets.h"
#include "mm_ui_state.h"
#include "mm_style.h"

#define METER_BG_OPA 51
#define METER_H_PCT 67
#define RIGHT_SPLACE MM_PORT_FONT_SIZE_SMALL*5

static void resizer_event_cb(lv_event_t * e)
{
    lv_obj_t * parent = lv_event_get_target(e);
    lv_obj_t * slider = lv_event_get_user_data(e);

    lv_coord_t w = lv_obj_get_width(parent);
    lv_obj_set_width(slider, w - RIGHT_SPLACE);
}

mm_widget_meter_t *mm_widget_meter_create(lv_obj_t *parent)
{
    struct mm_widget_meter_s *m = (struct mm_widget_meter_s *) malloc(sizeof(struct mm_widget_meter_s));
    MM_CHECK_MALLOC_E(m);

    m->visible = 1;
    mm_ui_state.active_meters++;

    lv_obj_t *obj = lv_obj_create(parent);
    m->container = obj;
    lv_obj_set_pos(obj, 0, -0);
    lv_obj_set_size(obj, LV_PCT(100), MM_FONT_SMALL->line_height);
    MM_STYLE_EMPTY_CONT(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_bar_create(parent_obj);
            m->bar = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_align(obj, LV_ALIGN_LEFT_MID);
            //lv_obj_set_size(obj, LV_PCT(METER_H_PCT), LV_PCT(70));
            lv_obj_set_size(obj, lv_obj_get_width(parent) - RIGHT_SPLACE, LV_PCT(90));
            lv_bar_set_range(obj, 0, 1001);
            lv_bar_set_value(obj, 0, LV_ANIM_OFF);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_LEFT_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, METER_BG_OPA, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(m->container, resizer_event_cb, LV_EVENT_SIZE_CHANGED, m->bar);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    m->bar_lbl = obj;
                    lv_obj_set_pos(obj, 8, -1);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, MM_PORT_FONT_SIZE_VERY_SMALL);
                    lv_obj_set_style_align(obj, LV_ALIGN_LEFT_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, mm_style_color_text(), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "-");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            m->side_lbl = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_align(obj, LV_ALIGN_RIGHT_MID);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_PCT(100));
            //lv_obj_set_style_min_width(obj, LV_PCT(100 - METER_H_PCT), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_min_width(obj, RIGHT_SPLACE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "-");
        }
    }

    return m;
}

void mm_widget_meter_del(mm_widget_meter_t *widget)
{
    lv_obj_del(widget->container);
    free(widget);
}

void mm_widget_meter_set_label(mm_widget_meter_t *widget, const char* text)
{
    lv_label_set_text(widget->bar_lbl, text);
}

static void __set_value(void *bar, int32_t v)
{
    lv_bar_set_value(bar, v, LV_ANIM_ON);
}

void mm_widget_meter_set_value(mm_widget_meter_t *widget, int value, const char* text)
{
    lv_label_set_text(widget->side_lbl, text);
    #if CONFIG_MM_UI_ANIMATED
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, __set_value);
    lv_anim_set_time(&a, 500 * CONFIG_MM_UI_ANIM_SPEED / 100);
    lv_anim_set_var(&a, widget->bar);
    lv_anim_set_values(&a, lv_bar_get_value(widget->bar), value);
    lv_anim_start(&a);
    #else
    __set_value(widget->bar, value);
    #endif
}

void mm_widget_meter_set_color(mm_widget_meter_t *widget, uint32_t color)
{
    lv_obj_set_style_bg_color(widget->bar, lv_color_hex(color), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(widget->bar, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(widget->bar_lbl,
            lv_color_mix(lv_color_hex(color), mm_style_color_background(), METER_BG_OPA),
            LV_PART_MAIN | LV_STATE_DEFAULT
    );
}

int mm_widget_meter_show(mm_widget_meter_t *widget)
{
    if(widget->visible) {
        return -1;
    }
    lv_obj_clear_flag(widget->container, LV_OBJ_FLAG_HIDDEN);
    widget->visible = 1;
    mm_ui_state.active_meters++;
    return 0;
}

int mm_widget_meter_hide(mm_widget_meter_t *widget)
{
    if(widget->visible) {
        lv_obj_add_flag(widget->container, LV_OBJ_FLAG_HIDDEN);
        widget->visible = 0;
        mm_ui_state.active_meters--;
        return 0;
    }
    return -1;
}
