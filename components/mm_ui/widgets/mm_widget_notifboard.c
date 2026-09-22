/**
 * @file
 *
 * @brief Panel listing notifications
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_notifboard.h"

#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_err.h"
#include "mm_port_ui_customization.h"
#include "mm_font.h"
#include "mm_ui_screen.h"
#include "mm_widgets.h"
#include "mm_ui_notification.h"
#include "mm_icon.h"
#include "mm_style.h"
#include "mm_ui.h"
#include "mm_ui_tile.h"

#if CONFIG_MM_PORT_HAS_TOUCHSCREEN

static void __event_clear_button_cb(lv_event_t* event)
{
    mm_ui_tile_switch(MM_UI_TILE_HOME, 1);
    mm_ui_snooze(NULL);
}

static void __event_clicked_cb(lv_event_t* event)
{
    struct mm_widget_notifboard_entry_s* m = (struct mm_widget_notifboard_entry_s*) lv_event_get_user_data(event);
    if (m->expanded) {
        m->expanded = 0;
        mm_widget_notifboard_entry_collapse(m);
    } else {
        m->expanded = 1;
        mm_widget_notifboard_entry_expand(m);
    }
}

static void __event_defocused_cb(lv_event_t* event)
{
    struct mm_widget_notifboard_entry_s* m = (struct mm_widget_notifboard_entry_s*)lv_event_get_user_data(event);
    mm_widget_notifboard_entry_collapse(m);
}

#endif /* CONFIG_MM_PORT_HAS_TOUCHSCREEN */

mm_widget_notifboard_t *mm_widget_notifboard_create(lv_obj_t *parent)
{
    struct mm_widget_notifboard_s *m;
    mm_widget_button_t *btn;
    lv_obj_t* obj;

    m = (struct mm_widget_notifboard_s *) malloc(sizeof(struct mm_widget_notifboard_s));
    MM_CHECK_MALLOC_E(m);

    /* main widget */
    obj = lv_list_create(parent);
    m->lst = obj;
    MM_STYLE_EMPTY_CONT(obj);
    lv_obj_set_style_pad_left(obj, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_base_dir(obj, LV_BASE_DIR_RTL, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    #if CONFIG_MM_PORT_HAS_TOUCHSCREEN

    /* button panel */
    obj = lv_obj_create(m->lst);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
    MM_STYLE_EMPTY_CONT(obj);
    lv_obj_set_style_pad_all(obj, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);

    btn = mm_widget_button_create(obj, MM_IMG_SRC_TRASH, CONFIG_MM_UI_BTN_SNOOZE_COLOR);
    lv_obj_add_event_cb(btn->button, __event_clear_button_cb, LV_EVENT_CLICKED, NULL);

    #else

    (void) btn;

    #endif /* CONFIG_MM_PORT_HAS_TOUCHSCREEN */

    return m;
}

void mm_widget_notifboard_delete(mm_widget_notifboard_t *widget)
{
    lv_obj_del(widget->lst);
    free(widget);
}

mm_widget_notifboard_entry_t *mm_widget_notifboard_entry_create(mm_widget_notifboard_t *widget, const char *text, const mm_img_src_t *img)
{
    struct mm_widget_notifboard_entry_s *m;
    uint32_t img_size;

    m = (struct mm_widget_notifboard_entry_s *) malloc(sizeof(struct mm_widget_notifboard_entry_s));
    MM_CHECK_MALLOC_E(m);

    /* panel_notif */
    lv_obj_t *obj = lv_obj_create(widget->lst);
    m->panel = obj;
    mm_add_style_panel(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_to_index(obj, 1);
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(obj, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, MM_PORT_FONT_SIZE_VERY_SMALL/4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, MM_PORT_FONT_SIZE_VERY_SMALL/4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);

    #if CONFIG_MM_PORT_HAS_TOUCHSCREEN
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(obj, __event_clicked_cb, LV_EVENT_CLICKED, m);
    lv_obj_add_event_cb(obj, __event_defocused_cb, LV_EVENT_DEFOCUSED, m);
    #endif

    /* icon */
    if(lv_img_src_get_type(img) == LV_IMG_SRC_VARIABLE) {
        img_size = MAX(((lv_img_header_t *) img)->w, ((lv_img_header_t *) img)->h);
    } else {
        img_size = MM_IMG_SRC_BELL->img_dsc->header.w;
    }

    obj = lv_img_create(m->panel);
    m->icon = obj;
    //lv_obj_set_size(obj, MM_PORT_FONT_SIZE_VERY_SMALL*3/2, MM_PORT_FONT_SIZE_VERY_SMALL*3/2);
    lv_img_set_src(obj, img);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_img_recolor(obj, mm_style_color_text(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_max_height(obj, 2+img_size*MM_PORT_NOTIF_ICON_SCALE/256, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_max_width(obj, 2+img_size*MM_PORT_NOTIF_ICON_SCALE/256, LV_PART_MAIN | LV_STATE_DEFAULT);
    #if CONFIG_MM_UI_ZOOM_IMAGE
    MM_IMAGE_SCALE_TO_SIZE(obj, img_size*MM_PORT_NOTIF_ICON_SCALE/256);
    #endif

    /* label */
    obj = lv_label_create(m->panel);
    m->lbl = obj;
    lv_obj_set_flex_grow(obj, 1);
    lv_obj_set_width(obj, LV_SIZE_CONTENT);
    //lv_obj_set_height(obj, LV_SIZE_CONTENT);
    //lv_obj_set_style_pad_top(obj, MM_PORT_FONT_SIZE_VERY_SMALL/2, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_pad_bottom(obj, MM_PORT_FONT_SIZE_VERY_SMALL/2, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_height(obj, MM_PORT_NOTIF_ENTRY_HEIGHT);
    lv_obj_set_style_text_font(obj, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_static(obj, text);
    //lv_obj_set_style_bg_opa(obj, 80, LV_PART_MAIN | LV_STATE_DEFAULT);

    mm_widget_notifboard_entry_collapse(m);
    lv_obj_scroll_to_y(widget->lst, lv_obj_get_height(lv_obj_get_child(widget->lst, 0))
            + lv_obj_get_style_pad_row(widget->lst, LV_PART_MAIN), LV_ANIM_OFF);

    m->expanded = 0;

    return m;
}

void mm_widget_notifboard_entry_delete(mm_widget_notifboard_entry_t *widget)
{
    lv_obj_del(widget->panel);
    free(widget);
}

void mm_widget_notifboard_entry_set_text(mm_widget_notifboard_entry_t* widget, const char* text)
{
    lv_label_set_text_static(widget->lbl, text);
}

void mm_widget_notifboard_entry_expand(mm_widget_notifboard_entry_t* widget)
{
    lv_obj_set_height(widget->lbl, LV_SIZE_CONTENT);
    lv_label_set_long_mode(widget->lbl, LV_LABEL_LONG_WRAP);
}

void mm_widget_notifboard_entry_collapse(mm_widget_notifboard_entry_t* widget)
{
    lv_obj_set_height(widget->lbl, MM_FONT_VERY_SMALL->line_height);
    lv_label_set_long_mode(widget->lbl, LV_LABEL_LONG_SCROLL);
}
