/**
 * @file
 *
 * @brief Notification popup widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_TOAST_H
#define __MM_WIDGET_TOAST_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_toast_s {
    lv_obj_t *panel;
    lv_obj_t *lbl;
    lv_timer_t *timer;
};

typedef struct mm_widget_toast_s mm_widget_toast_t;

mm_widget_toast_t *mm_widget_toast_create(lv_obj_t *parent);
void mm_widget_toast_del(mm_widget_toast_t *widget);

void mm_widget_toast_notify(mm_widget_toast_t* widget, unsigned long duration_ms, const char *msg);
void mm_widget_toast_cancel(mm_widget_toast_t* widget);

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_TOAST_H */
