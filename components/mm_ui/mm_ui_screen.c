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
    mm_ui_screen_create_touchpad();

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
}

lv_obj_t *mm_ui_screen_get_root_obj(mm_screen_t screen)
{
    MM_CHECK_E(screen < MM_N_SCREENS);
    return mm_screen_root[screen];
}
