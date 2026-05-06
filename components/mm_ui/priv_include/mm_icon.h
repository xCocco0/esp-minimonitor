/**
 * @file
 *
 * @brief UI icons
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_ICON_H
#define __MM_ICON_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MM_ICON_WIDTH 24

#define MM_ICON_ACTIVITY             ((mm_icon_t *) &mm_icon_activity)
#define MM_ICON_ALARM                ((mm_icon_t *) &mm_icon_alarm)
#define MM_ICON_APERTURE             ((mm_icon_t *) &mm_icon_aperture)
#define MM_ICON_ARROW                ((mm_icon_t *) &mm_icon_arrow)
#define MM_ICON_AT                   ((mm_icon_t *) &mm_icon_at)
#define MM_ICON_BADGE                ((mm_icon_t *) &mm_icon_badge)
#define MM_ICON_BAN                  ((mm_icon_t *) &mm_icon_ban)
#define MM_ICON_BATTERY              ((mm_icon_t *) &mm_icon_battery)
#define MM_ICON_BATTERY_SLASH        ((mm_icon_t *) &mm_icon_battery_slash)
#define MM_ICON_BED                  ((mm_icon_t *) &mm_icon_bed)
#define MM_ICON_BELL                 ((mm_icon_t *) &mm_icon_bell)
#define MM_ICON_BELL_SLASH           ((mm_icon_t *) &mm_icon_bell_slash)
#define MM_ICON_BLUETOOTH            ((mm_icon_t *) &mm_icon_bluetooth)
#define MM_ICON_BULLHORN             ((mm_icon_t *) &mm_icon_bullhorn)
#define MM_ICON_CAKE                 ((mm_icon_t *) &mm_icon_cake)
#define MM_ICON_CHECKMARK            ((mm_icon_t *) &mm_icon_checkmark)
#define MM_ICON_CLOCK                ((mm_icon_t *) &mm_icon_clock)
#define MM_ICON_CLOUD                ((mm_icon_t *) &mm_icon_cloud)
#define MM_ICON_COG                  ((mm_icon_t *) &mm_icon_cog)
#define MM_ICON_DOUBLE_ARROW_SLASH   ((mm_icon_t *) &mm_icon_double_arrow_slash)
#define MM_ICON_DOUBLE_ARROW         ((mm_icon_t *) &mm_icon_double_arrow)
#define MM_ICON_EMPTY_SUN            ((mm_icon_t *) &mm_icon_empty_sun)
#define MM_ICON_ENVELOPE             ((mm_icon_t *) &mm_icon_envelope)
#define MM_ICON_MOON                 ((mm_icon_t *) &mm_icon_moon)
#define MM_ICON_NUT                  ((mm_icon_t *) &mm_icon_nut)
#define MM_ICON_PHONE                ((mm_icon_t *) &mm_icon_phone)
#define MM_ICON_POWER                ((mm_icon_t *) &mm_icon_power)
#define MM_ICON_STOPWATCH            ((mm_icon_t *) &mm_icon_stopwatch)
#define MM_ICON_SUN                  ((mm_icon_t *) &mm_icon_sun)
#define MM_ICON_TRASH                ((mm_icon_t *) &mm_icon_trash)
#define MM_ICON_WHATSAPP             ((mm_icon_t *) &mm_icon_whatsapp)
#define MM_ICON_WIFI                 ((mm_icon_t *) &mm_icon_wifi)
#define MM_ICON_WIFI_SLASH           ((mm_icon_t *) &mm_icon_wifi_slash)

union mm_icon_u {
    lv_img_dsc_t img_dsc[0];
    const char file_path[0];
};

typedef union mm_icon_u mm_icon_t;

extern lv_img_dsc_t mm_icon_activity;
extern lv_img_dsc_t mm_icon_alarm;
extern lv_img_dsc_t mm_icon_aperture;
extern lv_img_dsc_t mm_icon_arrow;
extern lv_img_dsc_t mm_icon_at;
extern lv_img_dsc_t mm_icon_badge;
extern lv_img_dsc_t mm_icon_ban;
extern lv_img_dsc_t mm_icon_battery;
extern lv_img_dsc_t mm_icon_battery_slash;
extern lv_img_dsc_t mm_icon_bed;
extern lv_img_dsc_t mm_icon_bell;
extern lv_img_dsc_t mm_icon_bell_slash;
extern lv_img_dsc_t mm_icon_bluetooth;
extern lv_img_dsc_t mm_icon_bullhorn;
extern lv_img_dsc_t mm_icon_cake;
extern lv_img_dsc_t mm_icon_checkmark;
extern lv_img_dsc_t mm_icon_clock;
extern lv_img_dsc_t mm_icon_cloud;
extern lv_img_dsc_t mm_icon_cog;
extern lv_img_dsc_t mm_icon_double_arrow_slash;
extern lv_img_dsc_t mm_icon_double_arrow;
extern lv_img_dsc_t mm_icon_empty_sun;
extern lv_img_dsc_t mm_icon_envelope;
extern lv_img_dsc_t mm_icon_moon;
extern lv_img_dsc_t mm_icon_nut;
extern lv_img_dsc_t mm_icon_phone;
extern lv_img_dsc_t mm_icon_power;
extern lv_img_dsc_t mm_icon_stopwatch;
extern lv_img_dsc_t mm_icon_sun;
extern lv_img_dsc_t mm_icon_trash;
extern lv_img_dsc_t mm_icon_whatsapp;
extern lv_img_dsc_t mm_icon_wifi;
extern lv_img_dsc_t mm_icon_wifi_slash;

mm_icon_t *mm_icon_by_name(const char* name);
mm_icon_t *mm_icon_search_in_sdcard(const char* name);
mm_icon_t *mm_icon_search_in_flash(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* __MM_ICON_H */
