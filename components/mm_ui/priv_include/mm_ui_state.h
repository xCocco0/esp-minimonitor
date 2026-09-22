/**
 * @file
 *
 * @brief Store UI state
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_UI_STATE_H
#define __MM_UI_STATE_H

#include "mm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct __mm_ui_state_s {
    unsigned int active_meters;
    unsigned int pending_notifications;
    unsigned int panel_important;
    unsigned int dnd_enabled;
    unsigned int autorotation_enabled;
    mm_rotation_t current_rotation;
};

typedef struct __mm_ui_state_s __mm_ui_state_t;

extern __mm_ui_state_t mm_ui_state;

#ifdef __cplusplus
}
#endif

#endif /* __MM_UI_STATE_H */
