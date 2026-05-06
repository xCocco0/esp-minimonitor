/**
 * @file
 *
 * @brief Handle main tile layout changes
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_UI_LAYOUT_H
#define __MM_UI_LAYOUT_H

#ifdef __cplusplus
extern "C"
{
#endif

enum mm_layout_e {
    MM_LAYOUT_METERS_DUAL = 0,
    MM_LAYOUT_METERS_SINGLE,
    MM_LAYOUT_METERS_SINGLE_LARGE,
    MM_LAYOUT_NOMETERS_DUAL,
    MM_LAYOUT_NOMETERS_SINGLE,
    MM_N_LAYOUTS
};

typedef enum mm_layout_e mm_layout_t;

extern mm_layout_t mm_layout_current;

void mm_ui_layout_switch(mm_layout_t layout);

mm_layout_t mm_ui_layout_from_current_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_UI_LAYOUT_H */
