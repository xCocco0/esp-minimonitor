/**
 * @file
 *
 * @brief Handle panic screen instantiation
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_screen.h"
#include "sdkconfig.h"

#include "mm_err.h"
#include "mm_port_ui_customization.h"
#include "mm_widgets.h"
#include "mm_style.h"
#include "mm_font.h"

#if CONFIG_MM_PORT_HAS_TOUCHSCREEN
static void __event_clicked_cb(lv_event_t* event)
{
    MM_LOG_I("Leaving panic");
    mm_ui_screen_switch(MM_SCREEN_MAIN);
}
#endif

void mm_ui_screen_create_panic()
{
    lv_obj_t *screen_panic;
    lv_obj_t *panel_main;
    lv_obj_t *obj;

    /* panic screen */
    screen_panic = lv_obj_create(0);
    mm_screen_root[MM_SCREEN_PANIC] = screen_panic;
    mm_add_style_screen(screen_panic, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(screen_panic, LV_SCROLLBAR_MODE_OFF);

    /* main panel */
    panel_main = lv_obj_create(screen_panic);
    lv_obj_center(panel_main);
    lv_obj_set_size(panel_main, LV_PCT(100), LV_PCT(100));
    MM_STYLE_EMPTY_CONT(panel_main);
    lv_obj_set_style_pad_left(panel_main, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(panel_main, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(panel_main, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(panel_main, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

    #if CONFIG_MM_PORT_HAS_TOUCHSCREEN
    lv_obj_clear_flag(panel_main, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(panel_main, __event_clicked_cb, LV_EVENT_CLICKED, NULL);
    #endif

    /* qr */
    #define __QR_SIDE (MM_PORT_FONT_SIZE_VERY_SMALL*6)

    obj = lv_qrcode_create(panel_main, __QR_SIDE, mm_style_color_background(), mm_style_color_text());
    mm_widgets.lbl_panic_qr = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, __QR_SIDE, __QR_SIDE);
    lv_qrcode_update(obj, "", 1);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_align(obj, LV_ALIGN_BOTTOM_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* title label */
    #define __LEFT_COL_WIDTH (MM_PORT_DISPLAY_WIDTH - MM_PORT_BORDER_LEFT - MM_PORT_BORDER_RIGHT - MM_PORT_BORDER_INNER - __QR_SIDE)
    obj = lv_label_create(panel_main);
    mm_widgets.lbl_panic_title = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, LV_PCT(100), MM_PORT_FONT_SIZE_VERY_SMALL);
    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    mm_add_style_label(obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "");

    /* subtitle label */
    obj = lv_label_create(panel_main);
    mm_widgets.lbl_panic_subtitle = obj;
    lv_obj_set_pos(obj, 0, MM_PORT_FONT_SIZE_SMALL + MM_PORT_BORDER_INNER);
    if(MM_PORT_BORDER_TOP + MM_PORT_FONT_SIZE_SMALL + MM_PORT_FONT_SIZE_VERY_SMALL + MM_PORT_BORDER_INNER
            < MM_PORT_DISPLAY_HEIGHT - MM_PORT_BORDER_BOTTOM - __QR_SIDE) {
        /* fits above qr: use full width */
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
    } else {
        /* does not fit above qr: constrain it to the left size */
        lv_obj_set_size(obj, __LEFT_COL_WIDTH, LV_SIZE_CONTENT);
    }
    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "");

    /* continue label */
    obj = lv_label_create(panel_main);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, __LEFT_COL_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_style_align(obj, LV_ALIGN_BOTTOM_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "Press snooze or touch screen to continue.");
}
