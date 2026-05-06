/**
 * @file
 *
 * @brief Panel with notification icons
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_NOTIFTRAY_H
#define __MM_WIDGET_NOTIFTRAY_H

#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_icon.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_notiftray_s {
    lv_obj_t *container;
};

typedef struct mm_widget_notiftray_s mm_widget_notiftray_t;

struct mm_widget_notiftray_icon_s {
    lv_obj_t *lv_icon;
    lv_obj_t *lv_lbl;
    int count;
};

typedef struct mm_widget_notiftray_icon_s mm_widget_notiftray_icon_t;

mm_widget_notiftray_t *mm_widget_notiftray_create(lv_obj_t *parent);
void mm_widget_notiftray_delete(mm_widget_notiftray_t *widget);
int mm_widget_notiftray_n_icons(mm_widget_notiftray_t *widget);

mm_widget_notiftray_icon_t *mm_widget_notiftray_icon_create(mm_widget_notiftray_t *widget, const char *name, const mm_icon_t *img, int opt);
int mm_widget_notiftray_icon_inc(mm_widget_notiftray_icon_t *obj);
int mm_widget_notiftray_icon_sub(mm_widget_notiftray_icon_t *obj);
void mm_widget_notiftray_icon_delete(mm_widget_notiftray_icon_t *obj);

#if 0
mm_widget_notif_tray_t *mm_widget_notif_tray_create(lv_obj_t *parent);
void mm_widget_notif_tray_del(mm_widget_notif_tray_t *obj);

void mm_widget_notif_tray_push(mm_widget_notif_tray_t *widget, const char *name, const lv_img_dsc_t *img, int opt);
int mm_widget_notif_tray_cancel_by_name(mm_widget_notif_tray_t *widget, const char *name);
int mm_widget_notif_tray_cancel_all(mm_widget_notif_tray_t *widget);
void mm_widget_notif_tray_delete_all(mm_widget_notif_tray_t *widget);
void mm_widget_notif_tray_redraw(mm_widget_notif_tray_t *widget);

void notif_printall(mm_widget_notif_tray_t *widget);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_NOTIFTRAY_H */
