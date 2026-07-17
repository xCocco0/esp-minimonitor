/**
 * @file
 *
 * @brief UI widgets
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_WIDGETS_H
#define __MM_WIDGETS_H

#include <stdint.h>
#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_widget_meter.h"
#include "mm_widget_notiftray.h"
#include "mm_widget_notifboard.h"
#include "mm_widget_meterlist.h"
#include "mm_widget_glowframe.h"
#include "mm_widget_clock.h"
#include "mm_widget_toast.h"
#include "mm_widget_button.h"
#include "mm_widget_hidactions.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct __mm_widgets_s {
    /* main */
    lv_obj_t *panel_main;
    lv_obj_t *panel_main_top;
    lv_obj_t *panel_info;
    lv_obj_t *lbl_fps;
    lv_obj_t *panel_clock;
    mm_widget_glowframe_t *glowframe;
    mm_widget_meterlist_t *meter_list;
    mm_widget_notiftray_t *tray_notif;
    mm_widget_notifboard_t *board_notif;
    mm_widget_clock_t *clock;
    mm_widget_toast_t *toast;
    lv_obj_t *hidactions;
    lv_obj_t *lbl_no_hid;
    lv_obj_t *lbl_no_notif;
    lv_obj_t *slider_brightness;
    lv_obj_t *calendar;
    /* panic */
    lv_obj_t *lbl_panic_title;
    lv_obj_t *lbl_panic_subtitle;
    lv_obj_t *lbl_panic_qr;
};

typedef struct __mm_widgets_s __mm_widgets_t;

extern __mm_widgets_t mm_widgets;

struct __mm_tiles_s {
    lv_obj_t *tile_notif;
    lv_obj_t *tile_main;
    #if CONFIG_MM_IO_ENABLE_USB_OTG
    lv_obj_t *tile_hid;
    #endif
    #if CONFIG_MM_UI_USE_CALENDAR
    lv_obj_t *tile_calendar;
    #endif
};

#define MM_UI_N_TILES (sizeof(struct __mm_tiles_s)/sizeof(lv_obj_t*))

union __mm_tiles_u {
    struct __mm_tiles_s by_name;
    lv_obj_t *by_id[MM_UI_N_TILES];
};

typedef union __mm_tiles_u __mm_tiles_t;

extern __mm_tiles_t mm_tiles;


#ifdef __cplusplus
}
#endif

#endif /* __MM_WIDGETS_H */
