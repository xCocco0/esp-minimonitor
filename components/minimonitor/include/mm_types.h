/**
 * @file
 *
 * @brief Common type declarations
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_TYPES_H
#define __MM_TYPES_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ================= ui ================= */

typedef enum {
    MM_NOTIFY_DEFAULT = 0,
    MM_NOTIFY_STACK,
    MM_NOTIFY_REPLACE,
    MM_NOTIFY_APPEND,
} mm_notify_stack_mode_t;

struct mm_notify_param_s {
    const char *name;
    const char *icon;
    uint32_t color;
    uint32_t duration_ms;
    const char *msg;
    mm_notify_stack_mode_t opt;
    int persistent;
};

typedef enum {
        MM_TILE_CYCLE = 0,
        MM_TILE_SWIPE_LEFT,
        MM_TILE_SWIPE_RIGHT,
        MM_TILE_HOME,
} mm_tile_command_t;

typedef enum {
    MM_ROT_0 = 0,
    MM_ROT_90,
    MM_ROT_180,
    MM_ROT_270
} mm_rotation_t;

#define MM_ROTATION_TO_DEG(rot) ((int) (rot*90))
#define MM_DEG_TO_ROTATION(deg) ((mm_rotation_t) (deg/90))

typedef enum {
    MM_MOTION_ANCHOR_LANDSCAPE = 0,
    MM_MOTION_ANCHOR_PORTRAIT,
    MM_MOTION_ANCHOR_LANDSCAPE_INV,
    MM_MOTION_ANCHOR_PORTRAIT_INV,
} mm_motion_t;

/* ================= wifi ================= */

struct mm_setwifi_param_s {
    const char *ssid;
    const char *password;
};

struct mm_wifi_info_s {
    int connected;
    uint8_t bssid[6];
    uint8_t ssid[33];
    int8_t rssi;
    uint8_t ipv4[4];
    /* TODO: ipv6 */
};

/* ================= data request ================= */

enum mm_data_select_e {
    MM_DATA_WIFI_STATUS = 1,
    MM_DATA_ICON_LIST,
};

typedef enum mm_data_select_e mm_data_select_t;

struct mm_data_request_s {
    mm_data_select_t sel;
    SemaphoreHandle_t ready_sema;
    size_t *size; /**< bytes available at address ptr (not struct size). */
    void *ptr;
};

#ifdef __cplusplus
}
#endif

#endif /* __MM_TYPES_H */
