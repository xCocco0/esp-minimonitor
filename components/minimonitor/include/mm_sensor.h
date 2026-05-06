/**
 * @file
 *
 * @brief Utilities for handling sensor data and meter configs
 *
 * Copyright 2025 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_SENSOR_H
#define __MM_SENSOR_H

#include <stdint.h>
#include <stddef.h>
#include "mm_err.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CONFIG_MM_SENSOR_LABEL_MAX_LEN 64
#define CONFIG_MM_SENSOR_DESC_FMT_MAX_LEN 64
#define CONFIG_MM_SENSOR_PANEL_FMT_MAX_LEN 64

typedef enum {
        MM_METER_TYPE_DEFAULT =        (0),
#define MM_METER_TYPE_DEPENDS_ON(n)    ((mm_meter_type_t) ((n)+1))
        MM_METER_TYPE_PANEL =          (0x1 << 30),
        MM_METER_TYPE_WEAK =           (0x1 << 31),
} mm_meter_type_t;

struct mm_sensor_meter_config_s {
    int index;
    char label[CONFIG_MM_SENSOR_LABEL_MAX_LEN];
    char fmt[CONFIG_MM_SENSOR_DESC_FMT_MAX_LEN];
    uint32_t max_value;
    uint32_t timeout_ms;
    mm_meter_type_t type;
    int wakeup_arg;
    uint32_t wakeup_threshold;
    int color_arg;
    struct {
        uint32_t base,
                 warn,
                 alert;
    } color;
    struct {
        uint32_t low,
                 high;
    } threshold;
    int decimals;
    struct {
        uint32_t wakeup_threshold;
        char fmt[CONFIG_MM_SENSOR_PANEL_FMT_MAX_LEN];
    } panel;
};

#define MM_SENSOR_METER_CONFIG_DEFAULT { \
    .index = 0, \
    .label = "", \
    .fmt = "%d", \
    .max_value = 100, \
    .timeout_ms = 0, \
    .type = MM_METER_TYPE_DEFAULT, \
    .wakeup_arg = 0, \
    .wakeup_threshold = 0, \
    .color_arg = 0, \
    .color = { \
        .base = 0xff0000, \
        .warn = 0xff0000, \
        .alert = 0xff0000, \
    }, \
    .threshold = { \
        .low = ~0, \
        .high = ~0, \
    }, \
    .decimals = 0, \
    .panel = { \
        .wakeup_threshold = 0, \
        .fmt = "%d", \
    } \
}

struct mm_sensor_data_s {
    int index;
    uint32_t value1;
    uint32_t value2;
    uint32_t valueP;
};

#define MM_SENSOR_DECIMAL_FMT ".%4$0*3$u"
#define MM_SENSOR_PRINTF_ARGS(v1_integer, v1_decimals, v1_n_decimals, v2) v1_integer, v2, v1_n_decimals, v1_decimals

int mm_sensor_parse_fmt_str(char *d, const char *s, int decimals, size_t dsize);

mm_err_t mm_sensor_store_meter_config(struct mm_sensor_meter_config_s *config);
mm_err_t mm_sensor_fetch_meter_config(struct mm_sensor_meter_config_s *config);
mm_err_t mm_sensor_erase_meter_config(int index);

#ifdef __cplusplus
}
#endif

#endif /* __MM_SENSOR_H */
