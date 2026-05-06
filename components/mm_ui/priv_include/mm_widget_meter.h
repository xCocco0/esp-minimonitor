/**
 * @file
 *
 * @brief Meter widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_METER_H
#define __MM_WIDGET_METER_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_meter_s {
    lv_obj_t *container;
    lv_obj_t *bar;
    lv_obj_t *bar_lbl;
    lv_obj_t *side_lbl;
    int visible;
};

typedef struct mm_widget_meter_s mm_widget_meter_t;

mm_widget_meter_t *mm_widget_meter_create(lv_obj_t *parent);
void mm_widget_meter_del(mm_widget_meter_t *widget);

void mm_widget_meter_set_label(mm_widget_meter_t *widget, const char* text);
void mm_widget_meter_set_value(mm_widget_meter_t *widget, int value, const char* text);
void mm_widget_meter_set_color(mm_widget_meter_t *widget, uint32_t color);
int mm_widget_meter_show(mm_widget_meter_t *widget);
int mm_widget_meter_hide(mm_widget_meter_t *widget);

void mm_widget_meter_show_cpu(mm_widget_meter_t *widget, int cpuload, int cputemp);
void mm_widget_meter_show_gpu(mm_widget_meter_t *widget, int gpuload, int gputemp);
void mm_widget_meter_show_dram(mm_widget_meter_t *widget, uint32_t used_mb, uint16_t total_gb);


#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_METER_H */
