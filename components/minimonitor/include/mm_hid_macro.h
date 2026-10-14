/**
 * @file
 *
 * @brief HID macros parsing
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_HID_MACRO
#define __MM_HID_MACRO

#include <stdint.h>
#include "mm_hid_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_hid_macro_parser_data_s;
struct mm_hid_macro_parser_config_s;

struct mm_hid_macro_parser_s {
    const char *x;
    struct mm_hid_macro_parser_data_s *d;
    struct mm_hid_macro_parser_config_s *c;
};

typedef struct mm_hid_macro_parser_s mm_hid_macro_parser_t;

struct mm_hid_macro_parser_data_s {
    union {
        uint32_t num;
        int32_t snum;
        uint32_t timedelta;
        mm_key_t key_id;
        int32_t key;
        uint32_t wait;
        uint32_t macro_stmt;
        /* unused */
        void *key_combo;
    } match;
};

typedef void (*parser_callback_t)(struct mm_hid_macro_parser_s *p);

struct mm_hid_macro_parser_config_s {
    parser_callback_t parsed_macro_stmt_cb;
    parser_callback_t parsed_key_combo_cb;
    parser_callback_t parsed_wait_cb;
    parser_callback_t parsed_snum_cb;
    parser_callback_t parsed_key_cb;
    parser_callback_t parsed_timedelta_cb;
    parser_callback_t parsed_key_id_cb;
    parser_callback_t parsed_num_cb;
    void *user_data;
};

mm_hid_macro_parser_t *mm_hid_macro_parser_create(const struct mm_hid_macro_parser_config_s *config);
void mm_hid_macro_parser_destroy(mm_hid_macro_parser_t *p);
int mm_hid_macro_parse(mm_hid_macro_parser_t *p, const char *s);
int mm_hid_macro_test(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* __MM_HID_MACRO */
