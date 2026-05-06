/**
 * @file
 *
 * @brief Widget containing a list of meters
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_METERLIST_H
#define __MM_WIDGET_METERLIST_H

#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_widget_meter.h"
#include "mm_port_ui_customization.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_meterlist_s {
    lv_obj_t *container;
    //mm_widget_meter_t *meter[CONFIG_MM_MAX_METERS];
    struct {
        mm_widget_meter_t *w;
        uint32_t opt;
        int relevant;
    } meter[MM_PORT_MAX_METERS];
    uint16_t show_weak_meters;
};

typedef struct mm_widget_meterlist_s mm_widget_meterlist_t;

mm_widget_meterlist_t *mm_widget_meterlist_create(lv_obj_t *parent);
void mm_widget_meterlist_del(mm_widget_meterlist_t *widget);

void mm_widget_meterlist_meter_create(mm_widget_meterlist_t *widget, int index, uint32_t opt);
void mm_widget_meterlist_meter_del(mm_widget_meterlist_t *widget, int index);
void mm_widget_meterlist_meter_set_opt(mm_widget_meterlist_t *widget, int index, uint32_t opt);
void mm_widget_meterlist_meter_set_label(mm_widget_meterlist_t *widget, int index, const char* text);
void mm_widget_meterlist_meter_set_value(mm_widget_meterlist_t *widget, int index, int value, const char* text);
void mm_widget_meterlist_meter_set_color(mm_widget_meterlist_t *widget, int index, uint32_t color);
int mm_widget_meterlist_meter_show(mm_widget_meterlist_t *widget, int index);
int mm_widget_meterlist_meter_hide(mm_widget_meterlist_t *widget, int index);

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_METERLIST_H */
