/**
 * @file
 *
 * @brief Handle minimonitor events
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __MM_EVENT_H
#define __MM_EVENT_H

#include <time.h>
#include "mm_panic.h"
#include "mm_sensor.h"
#include "mm_types.h"
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    MM_EVENT_PERIODIC = 0,        /**< Pseudo-periodic handler (with `0 < period <= CONFIG_MM_EVENT_TASK_PERIOD_MS`) */
    MM_EVENT_SETTIME,             /**< Update system time */
    MM_EVENT_NOTIFY,              /**< Incoming system notification */
    MM_EVENT_SNOOZE,              /**< Snooze notifications */
    MM_EVENT_SETSENSOR,           /**< Received sensor reading */
    MM_EVENT_CONFSENSOR,          /**< Update sensor meter config */
    MM_EVENT_REBOOT,              /**< Reboot system */
    MM_EVENT_SNTPTRIGGER,         /**< Trigger SNTP time synchronization*/
    MM_EVENT_SETHOSTNAME,         /**< Update system hostname */
    MM_EVENT_SETTIMEZONE,         /**< Update timezone */
    MM_EVENT_SETWIFI,             /**< Update WiFi credentials */
    MM_EVENT_CHANGETILE,          /**< Trigger ui tile change */
    MM_EVENT_SETBACKLIGHT,        /**< Update backlight */
    MM_EVENT_BSOD,                /**< A panic was detected */
    MM_EVENT_STARTUP,             /**< Minimonitor started up */
    MM_EVENT_MOTION,              /**< IMU motion detected */

    MM_EVENT_MAX,
} mm_event_t;

typedef void (*mm_event_handler_t)(mm_event_t id, void *data);

void mm_event_init(void);
void mm_event_start(void);

void mm_event_queue(mm_event_t id, void *data);
void mm_event_add_handler(mm_event_t id, mm_event_handler_t func);

void mm_event_queue_settime(struct timeval *tv);
void mm_event_queue_notify(const char *name,
                                const char *icon,
                                uint32_t color,
                                uint32_t duration_ms,
                                const char *msg,
                                mm_notify_stack_mode_t opt,
                                int persistent);
void mm_event_queue_snooze(const char* name);
void mm_event_queue_setsensor(int index, uint32_t value1, uint32_t value2, uint32_t valueP);
void mm_event_queue_confsensor(struct mm_sensor_meter_config_s *param);
void mm_event_queue_reboot(void);
void mm_event_queue_sntptrigger(void);
void mm_event_queue_sethostname(const char *hostname);
void mm_event_queue_settimezone(const char *tz);
void mm_event_queue_setwifi(const char *ssid, const char *password);
void mm_event_queue_changetile(mm_tile_command_t cmd);
void mm_event_queue_setbacklight(uint8_t bk);
void mm_event_queue_bsod(struct mm_panic_info_s *param);
void mm_event_queue_motion(mm_motion_t motion);

#ifdef __cplusplus
}
#endif

#endif /* __MM_EVENT_H */
