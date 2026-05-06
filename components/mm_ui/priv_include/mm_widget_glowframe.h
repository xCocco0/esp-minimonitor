/**
 * @file
 *
 * @brief Glowing overlay widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_GLOWFRAME_H
#define __MM_WIDGET_GLOWFRAME_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_glowframe_s {
    lv_obj_t *container;
    int n_flashes;
    uint32_t color;
    lv_timer_t *flash_timer;
};

typedef struct mm_widget_glowframe_s mm_widget_glowframe_t;

mm_widget_glowframe_t *mm_widget_glowframe_create(lv_obj_t *parent);
void mm_widget_glowframe_del(mm_widget_glowframe_t *widget);

void mm_widget_glowframe_flash(mm_widget_glowframe_t* widget, unsigned long duration_ms, uint32_t color);
void mm_widget_glowframe_cancel(mm_widget_glowframe_t* widget);

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_GLOWFRAME_H */
