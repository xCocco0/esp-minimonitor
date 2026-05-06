/**
 * @file
 *
 * @brief Clock widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_CLOCK_H
#define __MM_WIDGET_CLOCK_H

#include <stdint.h>
#include <lvgl.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_widget_clock_s {
    lv_obj_t *container;
    lv_obj_t *lbl_hours;
    char buf_hours[4];
    lv_obj_t *lbl_colon;
    lv_obj_t *lbl_minutes;
    char buf_minutes[4];
    lv_obj_t *lbl_colon2;
    lv_obj_t *lbl_seconds;
    char buf_seconds[4];
    int separator_shown;
    lv_timer_t *refresh_time_timer;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

typedef struct mm_widget_clock_s mm_widget_clock_t;

mm_widget_clock_t *mm_widget_clock_create(lv_obj_t *parent);
void mm_widget_clock_del(mm_widget_clock_t *widget);

void mm_widget_clock_settime(mm_widget_clock_t *widget, uint8_t h, uint8_t min, uint8_t sec, uint16_t ms);


#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_CLOCK_H */
