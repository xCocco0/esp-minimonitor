/**
 * @file
 *
 * @brief Panel with notification icons
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_notiftray.h"

#include <lvgl.h>
#include "mm_err.h"
#include "mm_port_ui_customization.h"
#include "mm_font.h"
#include "mm_icon.h"
#include "mm_widgets.h"
#include "mm_style.h"
#include "mm_ui_screen.h"


#if CONFIG_MM_PORT_HAS_TOUCHSCREEN

static void __event_panel_pressed_cb(lv_event_t* event)
{
    mm_ui_screen_switch_tile(&mm_tiles.by_name.tile_notif, 1);
}

#endif

mm_widget_notiftray_t *mm_widget_notiftray_create(lv_obj_t *parent)
{
    lv_obj_t *obj;
    struct mm_widget_notiftray_s *m = (struct mm_widget_notiftray_s *) malloc(sizeof(struct mm_widget_notiftray_s));
    MM_CHECK_MALLOC_E(m);

    /* panel_notif */
    obj = lv_obj_create(parent);
    m->container = obj;
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    MM_STYLE_EMPTY_CONT(obj);
    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP_REVERSE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, MM_PORT_BORDER_INNER/2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, (MM_ICON_WIDTH*MM_PORT_NOTIF_ICON_SCALE/256)/4, LV_PART_MAIN | LV_STATE_DEFAULT);

    #if CONFIG_MM_PORT_HAS_TOUCHSCREEN
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(obj, __event_panel_pressed_cb, LV_EVENT_CLICKED, NULL);
    #endif

    return m;
}

void mm_widget_notiftray_delete(mm_widget_notiftray_t *widget)
{
    lv_obj_del(widget->container);
    free(widget);
}

int mm_widget_notiftray_n_icons(mm_widget_notiftray_t *widget)
{
    return lv_obj_get_child_cnt(mm_widgets.tray_notif->container);
}

mm_widget_notiftray_icon_t *mm_widget_notiftray_icon_create(mm_widget_notiftray_t *widget, const char *name, const mm_icon_t *img, int opt)
{
    MM_CHECK_E(name != NULL);
    MM_CHECK_E(img != NULL);

    /* allocate node */
    struct mm_widget_notiftray_icon_s *n = malloc(sizeof(struct mm_widget_notiftray_icon_s));
    MM_CHECK_MALLOC_E(n);

    /* populate struct fields */
    n->count = 1;

    /* create widget */
    lv_obj_t *obj;
    obj = lv_img_create(widget->container);
    lv_obj_move_to_index(obj, 0);
    n->lv_icon = obj;
    lv_obj_set_pos(obj, 0, 1);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_img_set_src(obj, img);
    lv_obj_set_style_img_recolor(obj, mm_style_color_text(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_max_height(obj, 2+MM_ICON_WIDTH*MM_PORT_NOTIF_ICON_SCALE/256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_max_width(obj, 2+MM_ICON_WIDTH*MM_PORT_NOTIF_ICON_SCALE/256, LV_PART_MAIN | LV_STATE_DEFAULT);
    #if CONFIG_MM_UI_ZOOM_IMAGE
    MM_IMAGE_SCALE_TO_SIZE(obj, MM_ICON_WIDTH*MM_PORT_NOTIF_ICON_SCALE/256);
    #endif
    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    obj = lv_label_create(n->lv_icon);
    n->lv_lbl = obj;
    #if CONFIG_MM_UI_NOTIF_COUNT_LABEL_LIMIT_100
    lv_obj_set_style_align(obj, LV_ALIGN_BOTTOM_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(obj, LV_PCT(33), 2);
    #else
    lv_obj_set_pos(obj, 0, 2);
    #endif
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(obj, MM_FONT_TINY, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_color_t colr = lv_obj_get_style_bg_color(widget->container, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, colr, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(obj, "");

    return n;
}

int mm_widget_notiftray_icon_inc(mm_widget_notiftray_icon_t *obj)
{
    char numstr[6];
    obj->count++;

    #if CONFIG_MM_UI_NOTIF_COUNT_LABEL_LIMIT_100
    if(obj->count < 100) {
        sprintf(numstr, "%d", obj->count % 100);
    } else {
        strcpy(numstr, "99+");
    }
    #else
    if(obj->count < 10) {
        sprintf(numstr, "%d", obj->count % 10);
    } else {
        strcpy(numstr, "9+");
    }
    #endif

    if(obj->count > 1) {
        lv_obj_clear_flag(obj->lv_lbl, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(obj->lv_lbl, numstr);

    return obj->count;
}

int mm_widget_notiftray_icon_sub(mm_widget_notiftray_icon_t *obj)
{
    char numstr[6];
    obj->count--;

    #if CONFIG_MM_UI_NOTIF_COUNT_LABEL_LIMIT_100
    if(obj->count < 100) {
        sprintf(numstr, "%d", obj->count % 100);
    } else {
        strcpy(numstr, "99+");
    }
    #else
    if(obj->count < 10) {
        sprintf(numstr, "%d", obj->count % 10);
    } else {
        strcpy(numstr, "9+");
    }
    #endif

    lv_label_set_text(obj->lv_lbl, numstr);

    if(obj->count <= 1) {
        lv_obj_add_flag(obj->lv_lbl, LV_OBJ_FLAG_HIDDEN);
    }

    return obj->count;
}

void mm_widget_notiftray_icon_delete(mm_widget_notiftray_icon_t *obj)
{
    lv_obj_del(obj->lv_icon);
    free(obj);
}
