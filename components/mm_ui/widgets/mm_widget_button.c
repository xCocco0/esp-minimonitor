/**
 * @file
 *
 * @brief Customized button widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_button.h"

#include "sdkconfig.h"
#include "mm_err.h"
#include "mm_port_ui_customization.h"
#include "mm_style.h"
#include "mm_icon.h"
#include "mm_font.h"
#include "mm_ui_screen.h"

static inline lv_obj_t *__add_icon(lv_obj_t *parent, mm_icon_t *ico)
{
    lv_obj_t *obj = lv_img_create(parent);
    lv_obj_center(obj);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_img_set_src(obj, ico);
    #if CONFIG_MM_UI_ZOOM_IMAGE
    lv_img_set_size_mode(obj, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(obj, MM_PORT_NOTIF_ICON_SCALE);
    #endif
    lv_obj_set_style_img_recolor(obj, mm_style_color_text(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    return obj;
}

static inline lv_obj_t *__add_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_center(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    return obj;
}

mm_widget_button_t *mm_widget_button_create(lv_obj_t *parent, mm_icon_t *ico, uint32_t color)
{
    struct mm_widget_button_s *m;
    lv_obj_t *obj;

    m = (struct mm_widget_button_s *) malloc(sizeof(struct mm_widget_button_s));
    MM_CHECK_MALLOC_E(m);

    /* button */
    obj = lv_btn_create(parent);
    m->button = obj;
    mm_add_style_panel(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(obj);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(obj, MM_PORT_BUTTON_SIZE, MM_PORT_BUTTON_SIZE);
    lv_obj_set_style_pad_all(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    mm_style_recolor_panel(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);

    // lv_obj_set_style_shadow_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_ofs_x(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_ofs_y(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_opa(obj, 48, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* icon inside button */
    m->icon = __add_icon(obj, ico);

    return m;
}

mm_widget_button_t *mm_widget_button_create_with_text(lv_obj_t *parent, const char* text, uint32_t color)
{
    struct mm_widget_button_s *m;
    lv_obj_t *obj;

    m = (struct mm_widget_button_s *) malloc(sizeof(struct mm_widget_button_s));
    MM_CHECK_MALLOC_E(m);

    /* button */
    obj = lv_btn_create(parent);
    m->button = obj;
    mm_add_style_panel(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(obj);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(obj, MM_PORT_BUTTON_SIZE, MM_PORT_BUTTON_SIZE);
    mm_style_recolor_panel(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* label inside button */
    m->icon = __add_label(obj, text);

    return m;
}

mm_widget_button_t *mm_widget_button_create_checkable(lv_obj_t *parent, mm_icon_t *ico, uint32_t color, int initial_state)
{
    struct mm_widget_button_s *m = mm_widget_button_create(parent, ico, color);

    lv_obj_add_flag(m->button, LV_OBJ_FLAG_CHECKABLE);
    mm_style_recolor_panel(m->button, CONFIG_MM_UI_BTN_DISABLED_COLOR, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(m->button, lv_color_hex(color), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(m->button, lv_color_hex(mm_style_color_shaded(color)), LV_PART_MAIN | LV_STATE_CHECKED);

    if(initial_state) {
        lv_obj_add_state(m->button, LV_STATE_CHECKED);
    }

    return m;
}

void mm_widget_button_del(mm_widget_button_t *widget)
{
    lv_obj_del(widget->button);
    free(widget);
}
