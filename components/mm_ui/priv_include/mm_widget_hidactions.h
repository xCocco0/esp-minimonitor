/**
 * @file
 *
 * @brief HID actions control widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGET_HIDACTIONS_H
#define __MM_WIDGET_HIDACTIONS_H

#include <lvgl.h>
#include "sdkconfig.h"

#include "mm_hid_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Hid actions main widget */

struct mm_widget_hidactions_s {
    lv_obj_t lst;
};

typedef struct mm_widget_hidactions_s mm_widget_hidactions_t;

const lv_obj_class_t mm_hidactions_class = {
    .base_class = &lv_list_class,
    .instance_size = sizeof(mm_widget_hidactions_t),
};

lv_obj_t *mm_widget_hidactions_create(lv_obj_t *parent);
void mm_widget_hidactions_delete(lv_obj_t *widget);

/* Hid actions controls (sub-widgets) */

struct mm_widget_hidactions_control_s {
    lv_obj_t panel;
    lv_timer_t *repeat_timer;
    uint32_t pressed;
    struct mm_hid_control_config_s conf;
    struct mm_hid_action_s act_m;
    struct mm_hid_action_s act_kb;
    struct mm_hid_action_s act_c;
};

typedef struct mm_widget_hidactions_control_s mm_widget_hidactions_control_t;

void mm_hidactions_control_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
void mm_hidactions_control_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

const lv_obj_class_t mm_hidactions_control_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = mm_hidactions_control_constructor,
    .destructor_cb = mm_hidactions_control_destructor,
    .instance_size = sizeof(mm_widget_hidactions_control_t),
};

struct mm_hid_control_config_s *mm_widget_hidaction_last_touchpad_config(void);

lv_obj_t *mm_widget_hidactions_control_add(lv_obj_t *parent, struct mm_hid_control_config_s *conf);
void mm_widget_hidactions_control_delete(lv_obj_t *obj, int index);

#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGET_HIDACTIONS_H */
