/**
 * @file
 *
 * @brief Manage UI tiles
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_UI_TILE_H
#define __MM_UI_TILE_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum mm_ui_tile_e {
    MM_UI_TILE_NOTIF = 0,
    MM_UI_TILE_HOME,
    #if CONFIG_MM_IO_ENABLE_USB_OTG
    MM_UI_TILE_HID,
    #endif
    #if CONFIG_MM_UI_USE_CALENDAR
    MM_UI_TILE_CALENDAR,
    #endif
    MM_UI_TILE_USER_0,
    #define MM_UI_TILE_USER_n(n) (MM_UI_TILE_USER_0 + (n))
};

typedef enum mm_ui_tile_e mm_ui_tile_t;

extern mm_ui_tile_t mm_ui_tile_current;

size_t mm_ui_tile_add(lv_obj_t *root);
size_t mm_ui_tile_add_at_index(lv_obj_t *root, size_t index);

lv_obj_t *mm_ui_tile_get_root_obj(mm_ui_tile_t tile);
size_t mm_ui_tile_get_count(void);

void mm_ui_tile_switch(mm_ui_tile_t tile, int animated);

#ifdef __cplusplus
}
#endif

#endif /* __MM_UI_TILE_H */
