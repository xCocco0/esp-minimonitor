/**
 * @file
 *
 * @brief Customized button widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_BUTTON_H
#define __MM_WIDGET_BUTTON_H

#include <lvgl.h>
#include "mm_icon.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_button_s {
    lv_obj_t *button;
    lv_obj_t *icon;
};

typedef struct mm_widget_button_s mm_widget_button_t;

mm_widget_button_t *mm_widget_button_create(lv_obj_t *parent, mm_icon_t *ico, uint32_t color);
mm_widget_button_t *mm_widget_button_create_checkable(lv_obj_t *parent, mm_icon_t *ico, uint32_t color, int initial_state);
void mm_widget_button_del(mm_widget_button_t *widget);

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_BUTTON_H */
