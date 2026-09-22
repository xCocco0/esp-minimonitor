/**
 * @file
 *
 * @brief UI styles syntax sugar
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_layout.h"

#include "mm_conf.h"
#include "mm_port_ui_customization.h"
#include "mm_widgets.h"
#include "mm_font.h"
#include "mm_ui_screen.h"
#include "mm_ui_state.h"
#include "mm_ui_tile.h"

#define __OVERFLOW 30

mm_layout_t mm_layout_current = MM_LAYOUT_NOMETERS_SINGLE;

void mm_ui_layout_switch(mm_layout_t layout)
{
    lv_obj_t *obj, *child;
    int mult;

    /* lbl_clock */

    obj = mm_widgets.clock->container;

    switch (layout) {
    case MM_LAYOUT_METERS_DUAL:
    case MM_LAYOUT_METERS_SINGLE:
        for(uint32_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
            child = lv_obj_get_child(obj, i);
            lv_obj_set_style_text_font(child, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        break;
    case MM_LAYOUT_METERS_SINGLE_LARGE:
    case MM_LAYOUT_NOMETERS_DUAL:
        for(uint32_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
            child = lv_obj_get_child(obj, i);
            lv_obj_set_style_text_font(child, MM_FONT_LARGE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        break;
    case MM_LAYOUT_NOMETERS_SINGLE:
        for(uint32_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
            child = lv_obj_get_child(obj, i);
            lv_obj_set_style_text_font(child, MM_FONT_VERY_LARGE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        break;
    default:
        break;
    }

    /* panel_clock */

    obj = mm_widgets.panel_clock;

    if(mm_conf_get_as_integer(MM_CONF_CLOCK_USE_SECONDS)) {
        mult = 140;
    } else {
        mult = 100;
    }

    switch (layout) {
    case MM_LAYOUT_METERS_DUAL:
        lv_obj_set_size(obj, MM_PORT_PANEL_NORMAL_WIDTH, MM_PORT_PANEL_NORMAL_HEIGHT);
        break;
    case MM_LAYOUT_METERS_SINGLE:
        lv_obj_set_size(obj, MM_PORT_PANEL_NORMAL_WIDTH, MM_PORT_PANEL_NORMAL_HEIGHT);
        break;
    case MM_LAYOUT_METERS_SINGLE_LARGE:
        lv_obj_set_size(obj, MM_PORT_PANEL_LARGE_WIDTH*mult/100, MM_PORT_PANEL_LARGE_HEIGHT);
        break;
    case MM_LAYOUT_NOMETERS_DUAL:
        lv_obj_set_size(obj, MM_PORT_PANEL_LARGE_WIDTH*mult/100, MM_PORT_PANEL_LARGE_HEIGHT);
        break;
    case MM_LAYOUT_NOMETERS_SINGLE:
        lv_obj_set_size(obj, MM_PORT_PANEL_VERY_LARGE_WIDTH*mult/100, MM_PORT_PANEL_VERY_LARGE_HEIGHT);
        break;
    default:
        break;
    }

    /* panel_info */

    obj = mm_widgets.panel_info;

    switch (layout) {
    case MM_LAYOUT_METERS_DUAL:
        lv_obj_set_parent(obj, mm_widgets.panel_main_top);
        lv_obj_set_size(obj, MM_PORT_PANEL_NORMAL_WIDTH, MM_PORT_PANEL_NORMAL_HEIGHT);
        lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        break;
    case MM_LAYOUT_NOMETERS_DUAL:
        lv_obj_set_parent(obj, mm_ui_tile_get_root_obj(MM_UI_TILE_HOME));
        lv_obj_set_pos(obj, 0, -MM_PORT_BORDER_TOP-__OVERFLOW);
        lv_obj_set_style_pad_top(obj, __OVERFLOW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(obj, MM_PORT_PANEL_NORMAL_WIDTH, MM_PORT_PANEL_NOTIF_TOP_HEIGHT+__OVERFLOW);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        break;
    case MM_LAYOUT_METERS_SINGLE:
    case MM_LAYOUT_METERS_SINGLE_LARGE:
    case MM_LAYOUT_NOMETERS_SINGLE:
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        break;
    default:
        break;
    }
}

mm_layout_t mm_ui_layout_from_current_state(void)
{
    const __mm_ui_state_t *s = &mm_ui_state;
    if(s->active_meters == MM_PORT_MAX_METERS) {
        if(mm_widget_notiftray_n_icons(mm_widgets.tray_notif) > 0 || s->panel_important) {
            return MM_LAYOUT_METERS_DUAL;
        } else {
            return MM_LAYOUT_METERS_SINGLE;
        }
    } else if(s->active_meters > 0) {
        if(mm_widget_notiftray_n_icons(mm_widgets.tray_notif) > 0 || s->panel_important) {
            return MM_LAYOUT_METERS_DUAL;
        } else {
            return MM_LAYOUT_METERS_SINGLE_LARGE;
        }
    } else {
        if(mm_widget_notiftray_n_icons(mm_widgets.tray_notif) > 0 || s->panel_important) {
            return MM_LAYOUT_NOMETERS_DUAL;
        } else {
            return MM_LAYOUT_NOMETERS_SINGLE;
        }
    }
}
