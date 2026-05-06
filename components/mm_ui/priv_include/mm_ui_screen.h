/**
 * @file
 *
 * @brief Handle LVGL screens
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_UI_SCREEN_H
#define __MM_UI_SCREEN_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum mm_screen_e {
    MM_SCREEN_MAIN = 0,
    MM_SCREEN_PANIC,
    MM_N_SCREENS,
};

typedef enum mm_screen_e mm_screen_t;

extern lv_obj_t *mm_screen_root[MM_N_SCREENS];
extern mm_screen_t mm_screen_current;

void mm_ui_screen_create_main(void);
void mm_ui_screen_create_panic(void);
void mm_ui_screen_create_all(void);

void mm_ui_screen_switch(mm_screen_t screen);

void mm_ui_screen_switch_tile_id(size_t id);
void mm_ui_screen_switch_tile(lv_obj_t **tile);


#ifdef __cplusplus
}
#endif

#endif /* __MM_UI_SCREEN_H */
