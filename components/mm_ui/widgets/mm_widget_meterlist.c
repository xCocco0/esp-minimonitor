/**
 * @file
 *
 * @brief Widget containing a list of meters
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_meterlist.h"

#include "sdkconfig.h"
#include "mm_sensor.h"
#include "mm_err.h"
#include "mm_types.h"
#include "mm_port_ui_customization.h"
#include "mm_font.h"
#include "mm_widget_meter.h"
#include "mm_ui_state.h"
#include "mm_widgets.h"
#include "mm_style.h"

mm_widget_meterlist_t *mm_widget_meterlist_create(lv_obj_t *parent)
{
    struct mm_widget_meterlist_s *m = (struct mm_widget_meterlist_s *) malloc(sizeof(struct mm_widget_meterlist_s));
    MM_CHECK_MALLOC_E(m);

    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, 0, 0);
    //lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
    MM_STYLE_EMPTY_CONT(obj);
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    m->container = obj;

    m->show_weak_meters = 0;

    return m;
}

void mm_widget_meterlist_del(mm_widget_meterlist_t *widget)
{
    lv_obj_del(widget->container);
    /* bars are automatically deleted as children */
    free(widget);
}

#if 0
void mm_widget_meterlist_move_to_index(mm_widget_meterlist_t *widget, lv_obj_t *obj, int index)
{
     lv_obj_move_to_index(obj, index);
    (void) widget;
}
#endif

void mm_widget_meterlist_meter_create(mm_widget_meterlist_t *widget, int index, uint32_t opt)
{
    mm_widget_meter_t *m;

    m = mm_widget_meter_create(widget->container);
    widget->meter[index].w = m;
    widget->meter[index].opt = opt;
    widget->meter[index].relevant = 0;
    mm_widget_meter_hide(widget->meter[index].w);
}

void mm_widget_meterlist_meter_del(mm_widget_meterlist_t *widget, int index)
{
    MM_CHECK_E(widget->meter[index].w != NULL);
    if(widget->meter[index].w->visible) {
        mm_ui_state.active_meters--;
    }
    mm_widget_meter_del(widget->meter[index].w);
    widget->meter[index].w = NULL;
}

void mm_widget_meterlist_meter_set_opt(mm_widget_meterlist_t *widget, int index, uint32_t opt)
{
    widget->meter[index].opt = opt;
    widget->show_weak_meters &= ~(1 << index);
    /* hide/show or show/hide to force update */
    if(widget->meter[index].relevant) {
        mm_widget_meterlist_meter_hide(widget, index);
        mm_widget_meterlist_meter_show(widget, index);
    } else {
        mm_widget_meterlist_meter_show(widget, index);
        mm_widget_meterlist_meter_hide(widget, index);
    }
}

void mm_widget_meterlist_meter_set_label(mm_widget_meterlist_t *widget, int index, const char* text)
{
    mm_widget_meter_set_label(widget->meter[index].w, text);
}

void mm_widget_meterlist_meter_set_value(mm_widget_meterlist_t *widget, int index, int value, const char* text)
{
    mm_widget_meter_set_value(widget->meter[index].w, value, text);
}

void mm_widget_meterlist_meter_set_color(mm_widget_meterlist_t *widget, int index, uint32_t color)
{
    mm_widget_meter_set_color(widget->meter[index].w, color);
}

static inline int __check_panel_important(mm_widget_meterlist_t *widget)
{
    for(int i = 0; i < MM_PORT_MAX_METERS; ++i) {
        if(widget->meter[i].opt & MM_METER_TYPE_PANEL) {
            /* has MM_METER_OPT_PANEL */
            if(widget->meter[i].relevant) {
                /* is relevant */
                return 1;
            }
        }
    }
    return 0;
}

static inline void __hide_weak_meters(mm_widget_meterlist_t *widget)
{
    for(int i = 0; i < MM_PORT_MAX_METERS; ++i) {
        if(widget->meter[i].opt & MM_METER_TYPE_WEAK) {
            mm_widget_meter_hide(widget->meter[i].w);
        }
    }
}

static inline void __show_weak_meters(mm_widget_meterlist_t *widget)
{
    for(int i = 0; i < MM_PORT_MAX_METERS; ++i) {
        if((widget->meter[i].opt & MM_METER_TYPE_WEAK) && widget->meter[i].relevant) {
            mm_widget_meter_show(widget->meter[i].w);
        }
    }
}

int mm_widget_meterlist_meter_show(mm_widget_meterlist_t *widget, int index)
{
    widget->meter[index].relevant = 1;

    if(widget->meter[index].opt & MM_METER_TYPE_WEAK) {
        /* weak meter */
        if(!widget->show_weak_meters) {
            /* cannot make it visible yet */
            return -1;
        }
    } else {
        /* not weak */
        if(widget->show_weak_meters == 0) {
            __show_weak_meters(widget);
        }
        widget->show_weak_meters |= (1 << index);
    }

    // if(widget->meter[index].opt & MM_METER_OPT_PANEL) {
    //     mm_ui_state.panel_important = 1;
    // }

    if(mm_widget_meter_show(widget->meter[index].w) == 0) {
        return 0;
    }
    return -1;
}

int mm_widget_meterlist_meter_hide(mm_widget_meterlist_t *widget, int index)
{
    widget->meter[index].relevant = 0;

    if(widget->meter[index].opt & MM_METER_TYPE_WEAK) {
        /* weak */
    } else {
        /* not weak, check if the remaining meters are
         * all weak and eventually hide them all */

        if(widget->show_weak_meters == (1 << index)) {
            /* this is the last non-weak meter,
             * hide all weak meters. */
            __hide_weak_meters(widget);
            widget->show_weak_meters = 0;
        } else {
            /* this is NOT the last non-weak meter */
            widget->show_weak_meters &= ~(1 << index);
        }
    }

    /* if no other widget with MM_METER_OPT_PANEL is shown,
        * mark the panel as not important, otherwise don't anything. */
    // if(widget->meter[index].opt & MM_METER_OPT_PANEL) {
    //     if(!__check_panel_important(widget)) {
    //         mm_ui_state.panel_important = 0;
    //     }
    // }

    if(mm_widget_meter_hide(widget->meter[index].w) == 0) {
        return 0;
    }
    return -1;
}
