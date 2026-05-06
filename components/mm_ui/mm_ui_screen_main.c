/**
 * @file
 *
 * @brief Handle main screen instantiation
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_ui_screen.h"

#include <time.h>
#include "minimonitor.h"
#include "mm_err.h"
#include "mm_conf.h"
#include "sdkconfig.h"
#include "mm_port_display.h"
#include "mm_port_ui_customization.h"
#include "mm_ui.h"
#include "mm_widgets.h"
#include "mm_font.h"
#include "mm_ui_layout.h"
#include "mm_ui_state.h"
#include "mm_icon.h"
#include "mm_style.h"

#if CONFIG_MM_PORT_HAS_TOUCHSCREEN

static void __slider_set_bright(lv_event_t *e)
{
        uint8_t bright;
        int released = (int) lv_event_get_user_data(e);

        bright = lv_slider_get_value(mm_widgets.slider_brightness);
        mm_setbacklight(bright);

        if(released) {
                mm_conf_set(MM_CONF_BACKLIGHT, &bright);
        }
}

static void __button_restart_press(lv_event_t *e)
{
        mm_setbacklight(0);
        mm_reboot();
}

static void __button_shutdown_press(lv_event_t *e)
{
        mm_shutdown();
}

static void __button_enablenotif_toggle(lv_event_t *e)
{
        mm_widget_button_t *btn = lv_event_get_user_data(e);
        int pressed = lv_obj_has_state(btn->button, LV_STATE_CHECKED);
        int dnd = !pressed;

        if(pressed) {
                lv_img_set_src(btn->icon, MM_ICON_BELL);
        } else {
                lv_img_set_src(btn->icon, MM_ICON_BELL_SLASH);
        }

        mm_ui_state.dnd_enabled = dnd;
        mm_conf_set(MM_CONF_DONOTDISTURB, &dnd);
}

#if CONFIG_MM_PORT_HAS_IMU

static void __button_autorot_toggle(lv_event_t *e)
{
        mm_widget_button_t *btn = lv_event_get_user_data(e);
        int pressed = lv_obj_has_state(btn->button, LV_STATE_CHECKED);

        if(pressed) {
                mm_ui_state.autorotation_enabled = 1;
                lv_img_set_src(btn->icon, MM_ICON_DOUBLE_ARROW);
        } else {
                mm_ui_state.autorotation_enabled = 0;
                lv_img_set_src(btn->icon, MM_ICON_DOUBLE_ARROW_SLASH);
        }

        mm_conf_set(MM_CONF_AUTOROTATION, &pressed);
}

#else

static void __button_rotate_press(lv_event_t *e)
{
        int rot_deg;
        mm_rotation_t rot;

        rot_deg = MM_ROTATION_TO_DEG(mm_ui_state.current_rotation);
        #if CONFIG_MM_PORT_DISABLE_ROTATION_PORTRAIT
        rot_deg = (rot_deg + 180) % 360;
        #else
        rot_deg = (rot_deg + 90) % 360;
        #endif
        rot = MM_DEG_TO_ROTATION(rot_deg);

        mm_ui_rotate_screen(rot);
}

#endif /* CONFIG_MM_PORT_HAS_IMU */

#endif /* CONFIG_MM_PORT_HAS_TOUCHSCREEN */

void mm_ui_screen_create_main()
{
        lv_obj_t *screen_main;
        lv_obj_t *panel_main;
        lv_obj_t *panel_main_top;
        lv_obj_t *panel_info;
        lv_obj_t *lbl_fps;
        lv_obj_t *panel_clock;
        mm_widget_glowframe_t *glowframe;
        mm_widget_notiftray_t *tray_notif;
        lv_obj_t *lbl_no_notif;
        mm_widget_notifboard_t *board_notif;
        mm_widget_meterlist_t *meter_list;
        mm_widget_clock_t *clock;
        mm_widget_toast_t *toast;
        #if CONFIG_MM_PORT_HAS_TOUCHSCREEN
        lv_obj_t *panel_control_top;
        lv_obj_t *panel_control_btn;
        lv_obj_t *icon_brightness_low;
        lv_obj_t *icon_brightness_high;
        lv_obj_t *slider_brightness;
        lv_obj_t *panel_bright;
        #endif

        lv_obj_t *tile_main, *tile_notif, *tile_control;
        #if CONFIG_MM_UI_USE_CALENDAR
        lv_obj_t *tile_calendar;
        lv_obj_t *calendar;
        #endif

        struct tm time;
        char *bg_img_name;
        mm_icon_t *bg_img;

        mm_gettime(&time);

        /* main screen */
        screen_main = lv_tileview_create(0);
        mm_screen_root[MM_SCREEN_MAIN] = screen_main;
        mm_add_style_screen(screen_main, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(screen_main, LV_SCROLLBAR_MODE_OFF);
        bg_img_name = (char *) mm_conf_get_with_malloc(MM_CONF_BACKGROUND_IMAGE);
        if(bg_img_name) {
                if(bg_img_name[0] != '\0') {
                        bg_img = mm_icon_search_in_sdcard(bg_img_name);
                        if(bg_img) {
                                lv_obj_set_style_bg_img_src(screen_main, bg_img, LV_PART_MAIN | LV_STATE_DEFAULT);
                        }
                }
                free(bg_img_name);
        }

        /* --------------------------------------------------------------------------------------- */

        /* main tile */
        tile_main = lv_tileview_add_tile(screen_main, 1, 1, LV_DIR_LEFT | LV_DIR_TOP
                        #if CONFIG_MM_UI_USE_CALENDAR
                        | LV_DIR_RIGHT
                        #endif
        );
        mm_tiles.by_name.tile_main = tile_main;
        lv_obj_set_size(tile_main, LV_PCT(100), LV_PCT(100));
        MM_STYLE_EMPTY_CONT(tile_main);
        lv_obj_set_style_pad_left(tile_main, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(tile_main, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(tile_main, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(tile_main, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* main panel */
        panel_main = lv_obj_create(tile_main);
        mm_widgets.panel_main = panel_main;
        lv_obj_center(panel_main);
        lv_obj_set_size(panel_main, LV_PCT(100), LV_PCT(100));
        MM_STYLE_EMPTY_CONT(panel_main);

        lv_obj_set_style_pad_row(panel_main, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(panel_main, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(panel_main, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(panel_main, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_track_place(panel_main, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(panel_main, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* panel main top */
        panel_main_top = lv_obj_create(panel_main);
        mm_widgets.panel_main_top = panel_main_top;
        MM_STYLE_EMPTY_CONT(panel_main_top);
        lv_obj_set_size(panel_main_top, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_layout(panel_main_top, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(panel_main_top, LV_FLEX_FLOW_ROW_REVERSE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(panel_main_top, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(panel_main_top, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* meter_list */
        meter_list = mm_widget_meterlist_create(panel_main);
        mm_widgets.meter_list = meter_list;
        lv_obj_set_size(meter_list->container, LV_PCT(100), LV_SIZE_CONTENT);

        for(int i = 0; i < MM_PORT_MAX_METERS; ++i) {
                mm_widget_meterlist_meter_create(meter_list, i, 0);
        }

        /* panel_clock */
        panel_clock = lv_obj_create(panel_main_top);
        mm_widgets.panel_clock = panel_clock;
        mm_add_style_panel(panel_clock, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(panel_clock, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(panel_clock, LV_SCROLLBAR_MODE_OFF);
        lv_obj_center(panel_clock);
        lv_obj_set_size(panel_clock, MM_PORT_PANEL_NORMAL_WIDTH, MM_PORT_PANEL_NORMAL_HEIGHT);
        lv_obj_set_grid_cell(panel_clock, LV_GRID_ALIGN_START, 1, 1,
                        LV_GRID_ALIGN_END, 0, 1);
        lv_obj_set_style_layout(panel_clock, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(panel_clock, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(panel_clock, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_track_place(panel_clock, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(panel_clock, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* clock */
        clock = mm_widget_clock_create(panel_clock);
        mm_widgets.clock = clock;
        lv_obj_center(clock->container);

        /* panel_info */
        panel_info = lv_obj_create(panel_main_top);
        mm_widgets.panel_info = panel_info;
        mm_add_style_panel(panel_info, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(panel_info, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(panel_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_align(panel_info, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(panel_info, MM_PORT_PANEL_NORMAL_WIDTH, MM_PORT_PANEL_NORMAL_HEIGHT);

        /* lbl_fps */
        lbl_fps = lv_label_create(panel_info);
        mm_widgets.lbl_fps = lbl_fps;
        mm_add_style_label(lbl_fps, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_pos(lbl_fps, 0, 0);
        lv_obj_set_size(lbl_fps, LV_SIZE_CONTENT, MM_PORT_FONT_SIZE_NORMAL);
        lv_label_set_long_mode(lbl_fps, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(lbl_fps, MM_FONT_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(lbl_fps, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_align(lbl_fps, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl_fps, "");

        /* panel_notif */
        tray_notif = mm_widget_notiftray_create(panel_info);
        mm_widgets.tray_notif = tray_notif;
        //mm_widgets.tray_notif.layouts = &layout_tray_notif[0];
        lv_obj_set_scrollbar_mode(tray_notif->container, LV_SCROLLBAR_MODE_OFF);

        /* toast */
        toast = mm_widget_toast_create(tile_main);
        mm_widgets.toast = toast;
        lv_obj_set_style_align(toast->panel, LV_ALIGN_BOTTOM_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_pos(toast->panel, 0, 0);
        lv_obj_set_size(toast->panel, LV_PCT(100), LV_SIZE_CONTENT);

        /* panel_glow */
        //glowframe = mm_widget_glowframe_create(tile_main);
        glowframe = mm_widget_glowframe_create(lv_layer_top());
        mm_widgets.glowframe = glowframe;
        lv_obj_clear_flag(glowframe->container, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_center(glowframe->container);
        lv_obj_set_size(glowframe->container, LV_PCT(100), LV_PCT(100));

        /* --------------------------------------------------------------------------------------- */

        /* tile_notif */
        tile_notif = lv_tileview_add_tile(screen_main, 0, 1, LV_DIR_RIGHT);
        MM_STYLE_EMPTY_CONT(tile_notif);
        mm_tiles.by_name.tile_notif = tile_notif;
        lv_obj_set_size(tile_notif, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_all(tile_notif, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        //MM_STYLE_EMPTY_CONT(tile_notif);

        /* board_notif */
        board_notif = mm_widget_notifboard_create(tile_notif);
        mm_widgets.board_notif = board_notif;
        lv_obj_set_size(board_notif->lst, LV_PCT(100), LV_PCT(100));
        //lv_obj_center(board_notif->lst);
        lv_obj_add_flag(board_notif->lst, LV_OBJ_FLAG_HIDDEN);

        /* lbl_no_notif */
        lbl_no_notif = lv_label_create(tile_notif);
        mm_widgets.lbl_no_notif = lbl_no_notif;
        lv_obj_center(lbl_no_notif);
        lv_label_set_text(lbl_no_notif, "No notifications");
        lv_obj_set_style_text_font(lbl_no_notif, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* --------------------------------------------------------------------------------------- */

        /* tile_control */
        tile_control = lv_tileview_add_tile(screen_main, 1, 0, LV_DIR_BOTTOM);
        MM_STYLE_EMPTY_CONT(tile_control);
        lv_obj_set_size(tile_control, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_layout(tile_control, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(tile_control, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_row(tile_control, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_left(tile_control, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_top(tile_control, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_right(tile_control, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_bottom(tile_control, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(tile_control, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_style_bg_opa(tile_control, LV_OPA_MAX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(tile_control, mm_style_color_background(), LV_PART_MAIN | LV_STATE_DEFAULT);

        #if CONFIG_MM_PORT_HAS_TOUCHSCREEN

        /* panel_control_top */
        panel_control_top = lv_obj_create(tile_control);
        MM_STYLE_EMPTY_CONT(panel_control_top);
        lv_obj_set_size(panel_control_top, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_flex_grow(panel_control_top, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(panel_control_top, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(panel_control_top, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(panel_control_top, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(panel_control_top, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* panel_bright */
        panel_bright = lv_obj_create(panel_control_top);
        lv_obj_center(panel_bright);
        lv_obj_set_size(panel_bright, LV_PCT(100), LV_SIZE_CONTENT);
        // lv_obj_set_style_pad_left(panel_bright, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_top(panel_bright, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_right(panel_bright, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        // lv_obj_set_style_pad_bottom(panel_bright, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(panel_bright, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(panel_bright, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(panel_bright, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(panel_bright, MM_PORT_BORDER_INNER*2, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* icon_brightness_low */
        icon_brightness_low = lv_img_create(panel_bright);
        lv_obj_set_size(icon_brightness_low, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_img_set_src(icon_brightness_low, MM_ICON_EMPTY_SUN);
        lv_obj_set_style_img_recolor(icon_brightness_low, mm_style_color_text(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_img_recolor_opa(icon_brightness_low, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        #if CONFIG_MM_UI_ZOOM_IMAGE
        lv_img_set_zoom(icon_brightness_low, MM_PORT_NOTIF_ICON_SCALE);
        lv_img_set_size_mode(icon_brightness_low, LV_IMG_SIZE_MODE_REAL);
        #endif

        /* slider_brightness */
        slider_brightness = lv_slider_create(panel_bright);
        mm_widgets.slider_brightness = slider_brightness;
        lv_slider_set_range(slider_brightness, 1, 255);
        lv_slider_set_value(slider_brightness, mm_conf_get_as_integer(MM_CONF_BACKLIGHT), LV_ANIM_OFF);
        lv_obj_set_size(slider_brightness, LV_SIZE_CONTENT, MM_PORT_FONT_SIZE_VERY_SMALL);
        lv_obj_set_style_flex_grow(slider_brightness, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(slider_brightness, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(slider_brightness, __slider_set_bright, LV_EVENT_VALUE_CHANGED, (void *) 0);
        lv_obj_add_event_cb(slider_brightness, __slider_set_bright, LV_EVENT_RELEASED, (void *) 1);

        /* icon_brightness_high */
        icon_brightness_high = lv_img_create(panel_bright);
        lv_obj_set_size(icon_brightness_high, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_img_set_src(icon_brightness_high, MM_ICON_SUN);
        lv_obj_set_style_img_recolor(icon_brightness_high, mm_style_color_text(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_img_recolor_opa(icon_brightness_high, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        #if CONFIG_MM_UI_ZOOM_IMAGE
        lv_img_set_zoom(icon_brightness_high, MM_PORT_NOTIF_ICON_SCALE);
        lv_img_set_size_mode(icon_brightness_high, LV_IMG_SIZE_MODE_REAL);
        #endif

        /* panel_control_btn */
        panel_control_btn = lv_obj_create(tile_control);
        MM_STYLE_EMPTY_CONT(panel_control_btn);
        lv_obj_set_size(panel_control_btn, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_flex_grow(panel_control_btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(panel_control_btn, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(panel_control_btn, LV_FLEX_FLOW_COLUMN_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(panel_control_btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_track_place(panel_control_btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(panel_control_btn, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(panel_control_btn, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(panel_control_btn, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(panel_control_btn, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(panel_control_btn, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        /* control buttons */
        mm_widget_button_t *btn_shutdown, *btn_reboot, *btn_enablenotif, *btn_rot;
        btn_shutdown = mm_widget_button_create(panel_control_btn, MM_ICON_POWER, CONFIG_MM_UI_BTN_SHUTDOWN_COLOR);
        lv_obj_add_event_cb(btn_shutdown->button, __button_shutdown_press, LV_EVENT_CLICKED, btn_shutdown);

        btn_reboot = mm_widget_button_create(panel_control_btn, MM_ICON_ARROW, CONFIG_MM_UI_BTN_REBOOT_COLOR);
        lv_obj_add_event_cb(btn_reboot->button, __button_restart_press, LV_EVENT_CLICKED, btn_reboot);

        btn_enablenotif = mm_widget_button_create_checkable(
                panel_control_btn,
                MM_ICON_BELL,
                CONFIG_MM_UI_BTN_ENABLENOTIF_COLOR,
                !mm_conf_get_as_integer(MM_CONF_DONOTDISTURB)
        );
        lv_obj_add_event_cb(btn_enablenotif->button, __button_enablenotif_toggle, LV_EVENT_VALUE_CHANGED, btn_enablenotif);
        lv_event_send(btn_enablenotif->button, LV_EVENT_VALUE_CHANGED, NULL);

        #if CONFIG_MM_PORT_HAS_IMU
        btn_rot = mm_widget_button_create_checkable(panel_control_btn,
                MM_ICON_DOUBLE_ARROW_SLASH,
                CONFIG_MM_UI_BTN_ROTATION_COLOR,
                mm_conf_get_as_integer(MM_CONF_AUTOROTATION)
        );
        lv_obj_add_event_cb(btn_rot->button, __button_autorot_toggle, LV_EVENT_VALUE_CHANGED, btn_rot);
        lv_event_send(btn_rot->button, LV_EVENT_VALUE_CHANGED, NULL);
        #else
        btn_rot = mm_widget_button_create(panel_control_btn, MM_ICON_DOUBLE_ARROW, CONFIG_MM_UI_BTN_ROTATION_COLOR);
        lv_obj_add_event_cb(btn_rot->button, __button_rotate_press, LV_EVENT_CLICKED, btn_rot);
        #endif
        //lv_obj_add_flag(child, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK)

        #endif

        /* --------------------------------------------------------------------------------------- */

        #if CONFIG_MM_UI_USE_CALENDAR

        tile_calendar = lv_tileview_add_tile(screen_main, 2, 1, LV_DIR_LEFT);
        mm_tiles.by_name.tile_calendar = tile_calendar;
        MM_STYLE_EMPTY_CONT(tile_calendar);
        lv_obj_set_size(tile_calendar, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_row(tile_calendar, MM_PORT_BORDER_INNER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(tile_calendar, MM_PORT_BORDER_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(tile_calendar, MM_PORT_BORDER_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(tile_calendar, MM_PORT_BORDER_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(tile_calendar, MM_PORT_BORDER_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

        calendar = lv_calendar_create(tile_calendar);
        mm_widgets.calendar = calendar;
        lv_obj_set_pos(calendar, 0, 0);
        lv_obj_set_size(calendar, LV_PCT(100), LV_PCT(100));
        // lv_calendar_set_today_date(calendar, 1900 + time.tm_year, 1 + time.tm_mon, time.tm_mday);
        // lv_calendar_set_showed_date(calendar, 1900 + time.tm_year, 1 + time.tm_mon);
        lv_obj_set_style_text_font(calendar, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(calendar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(calendar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        #else
        (void) time;
        #endif

        /* --------------------------------------------------------------------------------------- */

        mm_ui_layout_switch(MM_LAYOUT_METERS_DUAL);

        lv_scr_load(screen_main);
}
