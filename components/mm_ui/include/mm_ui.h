/**
 * @file
 *
 * @brief Minimonitor UI control pane
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_UI_H
#define __MM_UI_H

#include <stdint.h>
#include "mm_types.h"
#include "mm_sensor.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mm_ui_init(void);
int mm_ui_initialized(void);

uint32_t mm_ui_tick(void);

void mm_ui_notify(struct mm_notify_param_s *param);
void mm_ui_toast(const char* msg, uint32_t duration_ms);
void mm_ui_snooze(const char *name);

void mm_ui_set_time(uint8_t h, uint8_t min, uint8_t sec, uint16_t ms);

void mm_ui_set_meter_config(struct mm_sensor_meter_config_s *config);
void mm_ui_set_meter_enable(int index);
void mm_ui_set_meter_disable(int index);
void mm_ui_set_meter(int index, uint32_t value1, uint32_t value2, uint32_t valueP);

void mm_ui_rotate_screen(mm_rotation_t rot);

#ifdef __cplusplus
}
#endif

#endif /* __MM_UI_H */
