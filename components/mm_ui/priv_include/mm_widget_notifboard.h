/**
 * @file
 *
 * @brief Panel listing notifications
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_NOTIFBOARD_H
#define __MM_WIDGET_NOTIFBOARD_H

#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_icon.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_notifboard_s {
    lv_obj_t *lst;
};

typedef struct mm_widget_notifboard_s mm_widget_notifboard_t;

struct mm_widget_notifboard_entry_s {
    lv_obj_t *panel;
    lv_obj_t *lbl;
    lv_obj_t *icon;
    int expanded;
};

typedef struct mm_widget_notifboard_entry_s mm_widget_notifboard_entry_t;

mm_widget_notifboard_t *mm_widget_notifboard_create(lv_obj_t *parent);
void mm_widget_notifboard_delete(mm_widget_notifboard_t *widget);

mm_widget_notifboard_entry_t *mm_widget_notifboard_entry_create(mm_widget_notifboard_t *widget, const char *text, const mm_icon_t *img);
void mm_widget_notifboard_entry_delete(mm_widget_notifboard_entry_t *widget);

void mm_widget_notifboard_entry_set_text(mm_widget_notifboard_entry_t* widget, const char* text);
void mm_widget_notifboard_entry_expand(mm_widget_notifboard_entry_t* widget);
void mm_widget_notifboard_entry_collapse(mm_widget_notifboard_entry_t* widget);

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_NOTIFBOARD_H */
