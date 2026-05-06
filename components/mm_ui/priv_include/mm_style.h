/**
 * @file
 *
 * @brief UI styles syntax sugar
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_STYLE_H
#define __MM_STYLE_H

#include <lvgl.h>
#include "mm_macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MM_STYLE_EMPTY_CONT(obj) do { \
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); \
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); \
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); \
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT); \
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF); \
} while(0)

#define MM_IMAGE_SCALE_TO_SIZE(obj, size) do { \
    lv_img_set_size_mode((obj), LV_IMG_SIZE_MODE_REAL); \
    if( ((lv_img_t*) (obj))->w > 0 && ((lv_img_t*) (obj))->h > 0 ) { \
        lv_img_set_zoom((obj), 256*(size)/MIN(((lv_img_t*) (obj))->w, ((lv_img_t*) (obj))->h)); \
    } \
} while(0)

extern lv_style_t mm_style_label;
extern lv_style_t mm_style_panel;
extern lv_style_t mm_style_screen;

void mm_style_init(void);

void mm_add_style_label(lv_obj_t *obj, lv_style_selector_t selector);
void mm_add_style_panel(lv_obj_t *obj, lv_style_selector_t selector);
void mm_add_style_screen(lv_obj_t *obj, lv_style_selector_t selector);

void mm_style_recolor_panel(lv_obj_t *obj, uint32_t color, lv_style_selector_t selector);

uint32_t mm_style_color_shaded(uint32_t color);
lv_color_t mm_style_color_text(void);
lv_color_t mm_style_color_background(void);
lv_color_t mm_style_color_card(void);
lv_color_t mm_style_color_detail(void);
lv_color_t mm_style_color_primary(void);
lv_color_t mm_style_color_secondary(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_STYLE_H */
