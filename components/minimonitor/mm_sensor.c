/**
 * @file
 *
 * @brief Utilities for handling sensor data and meter configs
 *
 * Copyright 2025 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_sensor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mm_persist.h"
#include "mm_err.h"

/**
 * @brief Copy user format string to match expected format
 *
 * @param d[out] destination buffer
 * @param s[in] source buffer
 * @param decimals number of decimals that will be used
 * @param dsize size of the destination buffer (not string length)
 * @return MM_OK if the format was accepted or MM_FAIL if the format was rejected
 *
 * This function:
 * - adds the format specifier for the decimals after the first specifier
 * - discards the format strong if the specifier is not in the group [duoxX]
 * - discards * modifier for precision or width
 * - discards incorrect type length (only h,hh,l are accepted)
 * - checks the number of format specifiers
 */
mm_err_t mm_sensor_parse_fmt_str(char *d, const char *s, int decimals, size_t dsize)
{
    int match_count = 0;
    char *fmtspec_start;
    const char *d_last = d + (dsize-1);
    int dollar_m = 0;
    int last_arg = 0;
    mm_err_t error = MM_OK;

    while(*s != '\0' && d != d_last) {
        if(*s == '%') {
            fmtspec_start = d;
            dollar_m = 0;
            *d++ = *s++;
            if(*s == '%') {
                *d++ = *s++;
                continue;
            }
            /* $m operand number */
            if(*s == '1' || *s == '2') {
                *d++ = *s++;
                if(*s == '$') {
                    dollar_m = (int) (*(s-1) - '0');
                    *d++ = *s++;
                } else {
                    /* step back */
                    s--;
                    d--;
                }
            }
            /* width (* is excluded) */
            while('0' <= *s && *s <= '9') {
                *d++ = *s++;
            }
            /* precision (* is excluded) */
            if(*s == '.') {
                *d++ = *s++;
                while('0' <= *s && *s <= '9') {
                    *d++ = *s++;
                }
            }
            /* length */
            if(*s == 'h') {
                *d++ = *s++;
                if(*s == 'h') {
                    *d++ = *s++;
                }
            } else if(*s == 'l') {
                *d++ = *s++;
            }
            /* format pecifier [duoxX] */
            if(*s == 'd' || *s == 'u' || *s == 'o' || *s == 'x' || *s == 'X') {
                if(dollar_m) {
                    last_arg = dollar_m;
                } else {
                    last_arg = last_arg + 1;
                }
                if(last_arg < 3) {
                    match_count++;
                    *d++ = *s++;

                    if(decimals > 0 && last_arg == 1 && (d_last - d) >= sizeof(MM_SENSOR_DECIMAL_FMT)-1) {
                        strcpy(d, MM_SENSOR_DECIMAL_FMT);
                        // TODO: check out of bounds
                        d += sizeof(MM_SENSOR_DECIMAL_FMT)-1;
                    }

                    continue;
                }
            }
            /* invalid specifier: continue copying from last % symbol */
            error = MM_FAIL;
            d = fmtspec_start;
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';

    return (last_arg < 3) ? error : MM_FAIL;
}

/**
 * @brief Stores a meter config into nvs memory
 */
mm_err_t mm_sensor_store_meter_config(struct mm_sensor_meter_config_s *config)
{
    char key[32];
    /* store struct */
    sprintf(key, "meter%d", config->index);
    return mm_persist_store_obj(key, config, sizeof(struct mm_sensor_meter_config_s));
}

/**
 * @brief Retrieve a meter config from nvs memory
 *
 * @param config[in, out] Pointer to the struct to update
 *
 * @return MM_OK if the config was found and correctly updated, MM_FAIL otherwise
 * The input argument is used as both input and output. The index of the structure to fetch is the one written to
 * the argument before the call. After the call, the rest of the struct is updated with the values in nvs.
 */
mm_err_t mm_sensor_fetch_meter_config(struct mm_sensor_meter_config_s *config)
{
    char key[32];
    size_t size;

    /* store struct */
    sprintf(key, "meter%d", config->index);
    size = sizeof(struct mm_sensor_meter_config_s);
    if(mm_persist_fetch_obj(key, config, &size) == MM_FAIL) {
        return MM_FAIL;
    }

    return MM_OK;
}

/**
 * @brief Erases a meter config from nvs memory
 */
mm_err_t mm_sensor_erase_meter_config(int index)
{
    char key[32];
    sprintf(key, "meter%d", index);
    return mm_persist_invalidate_obj(key);
}
