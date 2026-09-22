/**
 * @file
 *
 * @brief Manage UI tiles
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_tile.h"

#include <stdlib.h>
#include <lvgl.h>
#include "mm_err.h"
#include "mm_ui_screen.h"

/* note: this variable is not updated when the user swipes on the UI */
mm_ui_tile_t mm_ui_tile_current = 0;

static lv_obj_t **s_tile_roots = NULL;
static size_t s_tile_roots_len = 0;

size_t mm_ui_tile_add(lv_obj_t *root)
{
    s_tile_roots_len++;
    /* note: yes realloc has overhead, but we just
       run this function at init so that's fine */
    s_tile_roots = realloc(s_tile_roots, sizeof(*s_tile_roots) * s_tile_roots_len);
    MM_CHECK_MALLOC_E(s_tile_roots);

    s_tile_roots[s_tile_roots_len] = root;

    return s_tile_roots_len;
}

size_t mm_ui_tile_add_at_index(lv_obj_t *root, size_t index)
{
    /* if element index is the last element, the size of the array is index + 1 */
    if(index + 1 > s_tile_roots_len) {
        s_tile_roots_len = index + 1;

        s_tile_roots = realloc(s_tile_roots, sizeof(*s_tile_roots) * s_tile_roots_len);
        MM_CHECK_MALLOC_E(s_tile_roots);
    }

    s_tile_roots[index] = root;

    return s_tile_roots_len;
}

lv_obj_t *mm_ui_tile_get_root_obj(mm_ui_tile_t tile)
{
    const size_t index = (size_t) tile;
    MM_CHECK_E(index < s_tile_roots_len);
    return s_tile_roots[index];
}

size_t mm_ui_tile_get_count(void)
{
    return s_tile_roots_len;
}

void mm_ui_tile_switch(mm_ui_tile_t tile, int animated)
{
    const size_t index = (size_t) tile;
    mm_ui_tile_current = index;
    if(index < s_tile_roots_len) {
        lv_obj_set_tile(mm_ui_screen_get_root_obj(MM_SCREEN_MAIN), s_tile_roots[index], CONFIG_MM_UI_ANIMATED ? animated : LV_ANIM_OFF);
    } else {
        MM_LOG_E("Tile %d is out of bounds", tile);
    }
}
