/**
 * @file
 *
 * @brief Main Minimonitor functions
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __MINIMONITOR_H
#define __MINIMONITOR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include "mm_types.h"
#include "mm_sensor.h"
#include "mm_hid_control.h"
#include "mm_err.h"

void mm_init(void);
int mm_initialized(void);

void mm_start(void);
int mm_started(void);

void mm_stop(void);

void mm_settime(struct timeval *tv);
void mm_gettime(struct tm *timeinfo);
void mm_notify(const char *name,
                    const char *icon,
                    uint32_t color,
                    uint32_t duration_ms,
                    const char *msg,
                    mm_notify_stack_mode_t opt,
                    int persistent);
void mm_toast(const char *msg);
void mm_snooze(const char* name);
void mm_setsensor(int index, uint32_t value1, uint32_t value2, uint32_t valueP);
void mm_confsensor(struct mm_sensor_meter_config_s *config);
void mm_confhid(struct mm_hid_control_config_s *config);
void mm_reboot(void);
void mm_shutdown(void);
void mm_sntptrigger(void);
void mm_sethostname(const char *hostname);
size_t mm_gethostname(const char *hostname, size_t size);
void mm_settimezone(const char *tz);
void mm_setwifi(const char *ssid, const char *password);
int mm_wifistatus(void);
void mm_tile_cycle(void);
void mm_tile_left(void);
void mm_tile_right(void);
void mm_tile_home(void);
void mm_setbacklight(uint8_t bk);
void mm_detected_motion(mm_motion_t motion);

#ifdef __cplusplus
}
#endif

#endif /* __MINIMONITOR_H */
