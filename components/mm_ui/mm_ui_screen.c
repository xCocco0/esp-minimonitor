/**
 * @file
 *
 * @brief Handle LVGL screens
 *
 * @note Screen-specific definitions are placed in mm_io_screen_NAME.c sources
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_screen.h"

#include "mm_err.h"
#include "mm_widgets.h"
#include "mm_ui_state.h"

lv_obj_t *mm_screen_root[MM_N_SCREENS] = {0};
mm_screen_t mm_screen_current = MM_SCREEN_MAIN;

void mm_ui_screen_create_all(void)
{
    static int init = 0;

    MM_CHECK_E(init == 0);

    mm_ui_screen_create_main();
    mm_ui_screen_create_panic();

    mm_ui_screen_switch_tile(&mm_tiles.by_name.tile_main);

    init = 1;
}

void mm_ui_screen_switch(mm_screen_t screen)
{
    MM_LOG_I("Switching to screen %d", screen);

    if(mm_screen_current == screen) {
        return;
    }

    lv_scr_load(mm_screen_root[screen]);
    mm_screen_current = screen;

    lv_obj_set_tile(mm_screen_root[MM_SCREEN_MAIN], mm_tiles.by_id[mm_ui_state.current_tile_id], CONFIG_MM_UI_ANIMATED ? LV_ANIM_ON : LV_ANIM_OFF);
}

void mm_ui_screen_switch_tile_id(size_t id)
{
    mm_ui_state.current_tile_id = id;
    lv_obj_set_tile(mm_screen_root[MM_SCREEN_MAIN], mm_tiles.by_id[id], CONFIG_MM_UI_ANIMATED ? LV_ANIM_ON : LV_ANIM_OFF);
}

void mm_ui_screen_switch_tile(lv_obj_t **tile)
{
    size_t id = ((void *) tile - (void *) &mm_tiles)/sizeof(mm_tiles.by_id[0]);
    if(id < MM_UI_N_TILES) {
        mm_ui_screen_switch_tile_id(id);
    }
}
