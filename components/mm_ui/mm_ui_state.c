/**
 * @file
 *
 * @brief Store UI state
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_state.h"

#include "mm_widgets.h"

__mm_ui_state_t mm_ui_state = {
    .active_meters = 0,
    .pending_notifications = 0,
    .panel_important = 0,
    .current_tile_id = 0,
    .dnd_enabled = 0,
    .autorotation_enabled = 0,
    .current_rotation = 0,
};
