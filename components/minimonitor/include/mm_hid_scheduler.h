/**
 * @file
 *
 * @brief Schedule HID commands
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_HID_SCHEDULER
#define __MM_HID_SCHEDULER

#include <time.h>
#include "mm_list.h"
#include "mm_hid_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mm_hid_scheduler_init(void);
uint32_t mm_hid_scheduler_schedule(void);
int mm_hid_scheduler_schedule_next(struct mm_hid_action_s *ev, uint32_t *timeout_ms);
int mm_hid_scheduler_in_queue(void);

uint32_t mm_hid_scheduler_queue_raw(mm_key_t key, int key_down, int16_t magnitude);
uint32_t mm_hid_scheduler_queue_macro(const char *macro);
uint32_t mm_hid_scheduler_queue_periodic(const char *macro, uint32_t repeats_count, uint32_t period_ms, uint32_t jitter_ms);
uint32_t mm_hid_scheduler_queue_down(const char *macro);
uint32_t mm_hid_scheduler_queue_up(const char *macro);

int mm_hid_scheduler_cancel(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* __MM_HID_SCHEDULER */
