/**
 * @file
 *
 * @brief HID macros parsing
 *
 * The grammar that describe a macro is the following:
 *
 * ```
 * MACRO      := MACRO_STMT [ ";" MACRO_STMT ]*
 * MACRO_STMT := ( KEY_COMBO [ "~" TIMEDELTA ] | WAIT )
 * KEY_COMBO  := KEY [ "+" KEY ]*
 * WAIT       := "Wait" [ "~" TIMEDELTA ]
 * SNUM       := [ "+" | "-" ] NUM
 * KEY        := KEY_ID [ "(" NUM ")" ]
 * TIMEDELTA  := NUM ( "s" | "ms" )
 * KEY_ID     := "A" | "B" | ... | "F1" | ";" | ... | "KPad(" | ... | "Vol+" | ... // may contain semicolor or parantesis
 * NUM        := [0-9]+
 * ```
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_hid_macro.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "mm_hid_control.h"

#define __DEBUG 0

#define __C(n) "\x1b[" #n "m"
#define __FMT __C(0;37)"["__C(0)"%20s"__C(0;37)"]"__C(0)""

#if __DEBUG
#define __DEBUG_PRINT_FN printf
#define __DEBUG_PRINT_ACCEPTED(p) __DEBUG_PRINT_FN("["__C(32)"Accepted"__C(0)"] %-20s: "__FMT"\n", __func__, (p)->x);
#define __DEBUG_PRINT_REJECTED(p) __DEBUG_PRINT_FN("["__C(31)"Rejected"__C(0)"] %-20s: "__FMT"\n", __func__, (p)->x);
#else
#define __DEBUG_PRINT_FN(...)
#define __DEBUG_PRINT_ACCEPTED(p) (void) p
#define __DEBUG_PRINT_REJECTED(p) (void) p
#endif

#define __ADVANCE(p, n) do { \
    for(int __i = (n); (__i) > 0; --(__i)) { \
        (p)->x++; \
        if(*(p)->x == '\0') { \
            __REJECT(); \
        } \
    } \
} while(0)

/* this garbage is to suppress GCC's Werror: unused-label */
#define __ACCEPT_LABEL goto accept; accept
#define __REJECT_LABEL goto reject; reject

#define __ACCEPT() goto accept
#define __REJECT() goto reject

#define __CALL_CB(f, p) do { \
    if(f) { \
        f(p); \
    } \
} while(0);

#define __IS_0_9(c) ('0' <= (c) && (c) <= '9')
#define __IS_1_9(c) ('1' <= (c) && (c) <= '9')
#define __CHAR_TO_NUM(c) ((c) - '0')

/* leaves */
static bool __parse_token(struct mm_hid_macro_parser_s *p, const char *tok);
static bool __parse_num(struct mm_hid_macro_parser_s *p);
/* nodes */
static bool __parse_key(struct mm_hid_macro_parser_s *p);
static bool __parse_key_id(struct mm_hid_macro_parser_s *p);
static bool __parse_snum(struct mm_hid_macro_parser_s *p);
static bool __parse_wait(struct mm_hid_macro_parser_s *p_start);
static bool __parse_timedelta(struct mm_hid_macro_parser_s *p_start);
static bool __parse_key_combo(struct mm_hid_macro_parser_s *p);
static bool __parse_macro_stmt(struct mm_hid_macro_parser_s *p);
static bool __parse_macro(struct mm_hid_macro_parser_s *p);

/* Parser implementation */

static bool __parse_token(struct mm_hid_macro_parser_s *p_start, const char *tok)
{
    size_t len = strlen(tok);

    if(strncmp(p_start->x, tok, len) == 0) {
        p_start->x += len;
        __ACCEPT();
    }

__REJECT_LABEL:
    __DEBUG_PRINT_FN("["__C(31)"Rejected"__C(0)"] %-20s: "__FMT" [Expected: %s]\n", __func__, p_start->x, tok);
    return 0;
__ACCEPT_LABEL:
    __DEBUG_PRINT_FN("["__C(32)"Accepted"__C(0)"] %-20s: "__FMT" [Consumed: %s]\n", __func__, p_start->x, tok);
    return 1;
}

/* NUM := [0-9]+ */
static bool __parse_num(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;
    uint32_t match = 0;

    if(__IS_0_9(*p.x)) {
        match += __CHAR_TO_NUM(*p.x);
        __ADVANCE(&p, 1);
    } else {
        __REJECT();
    }

    while(__IS_0_9(*p.x)) {
        match *= 10;
        match += __CHAR_TO_NUM(*p.x);
        __ADVANCE(&p, 1);
    }

    p_start->d->match.num = match;

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_num_cb, &p);
    __DEBUG_PRINT_FN("["__C(32)"Accepted"__C(0)"] %-20s: "__FMT" [Consumed: %.*s]\n", __func__, p.x, (int) (p.x - p_start->x), p_start->x);
    *p_start = p;
    //__DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

static bool __parse_key_id(struct mm_hid_macro_parser_s *p_start)
{
    mm_key_t k = 0;

    k = mm_hid_control_find_key_prefix(p_start->x);
    if(k) {
        p_start->d->match.key_id = k;
        p_start->x += MM_KEY_TO_STR_LEN(k);
    } else {
        __REJECT();
    }

__ACCEPT_LABEL:
    __CALL_CB(p_start->c->parsed_key_id_cb, p_start);
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* TIMEDELTA := NUM ( "s" | "ms" ) */
static bool __parse_timedelta(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;
    uint32_t num = 0;

    if(__parse_num(&p)) {
        num = p.d->match.num;
    } else {
        __REJECT();
    }

    if(__parse_token(&p, "s")) {
        p.d->match.timedelta = 1000*num;
    } else if(__parse_token(&p, "ms")) {
        p.d->match.timedelta = num;
    } else {
        __REJECT();
    }

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_timedelta_cb, &p);
    __DEBUG_PRINT_FN("["__C(32)"Accepted"__C(0)"] %-20s: "__FMT" [Consumed: %.*s]\n", __func__, p.x, (int) (p.x - p_start->x), p_start->x);
    *p_start = p;
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* SNUM := [ "+" | "-" ] NUM */
static bool __parse_snum(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;
    uint32_t match;
    int negative = 0;

    if(__parse_token(&p, "+")) {
        negative = 0;
    } else if(__parse_token(&p, "-")) {
        negative = 1;
    }

    if(__parse_num(&p)) {
        match = p.d->match.num;
        p.d->match.snum = negative ? -match : match;
    } else {
        __REJECT();
    }

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_snum_cb, &p);
    *p_start = p;
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* KEY := KEY_ID [ "(" NUM ")" ] */
static bool __parse_key(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;

    if(__parse_key_id(&p)) {
        /* pass */
    } else {
        __REJECT();
    }

    /* [ "(" NUM ")" ] */
    if(__parse_token(&p, "(")) {

        if(__parse_snum(&p)) {
            p.d->match.key = p.d->match.snum;
        } else {
            __REJECT();
        }

        if(__parse_token(&p, ")")) {
            /* pass */
        } else {
            __REJECT();
        }
    } else {
        p.d->match.key = 0;
    }

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_key_cb, &p);
    *p_start = p;
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* WAIT := "Wait" [ "~" TIMEDELTA ] */
static bool __parse_wait(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;

    if(__parse_token(&p, "Wait")) {
        /* pass */
    } else {
        __REJECT();
    }

    /* [ "~" TIMEDELTA ] */
    if(__parse_token(&p, "~")) {

        if(__parse_timedelta(&p)) {
            p.d->match.wait = p.d->match.timedelta;
        } else {
            __REJECT();
        }
    }

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_wait_cb, &p);
    *p_start = p;
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* KEY_COMBO := KEY [ "+" KEY ]* */
static bool __parse_key_combo(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;

    while(1) {
        if(__parse_key(&p)) {
            /* pass */
        } else {
            __REJECT();
        }

        if(__parse_token(&p, "+")) {
            /* pass */
        } else {
            break;
        }
    }

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_key_combo_cb, &p);
    *p_start = p;
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* MACRO_STMT := ( KEY_COMBO [ "~" TIMEDELTA ] | WAIT ) */
static bool __parse_macro_stmt(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;

    /* important: we need to check first for wait, then for keys, because otherwise
       it will match "Wait" as "W" + "ait" and fail */
    if(__parse_wait(&p)) {
        p.d->match.macro_stmt = p.d->match.wait;
    } else if(__parse_key_combo(&p)) {
        if(__parse_token(&p, "~")) {
            if(__parse_timedelta(&p)) {
                p.d->match.macro_stmt = p.d->match.timedelta;
            } else {
                __REJECT();
            }
        } else {
            p.d->match.macro_stmt = CONFIG_MM_HID_DEFAULT_PRESS_DURATION_MS;
        }
    } else {
        __REJECT();
    }

__ACCEPT_LABEL:
    __CALL_CB(p.c->parsed_macro_stmt_cb, &p);
    *p_start = p;
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

/* MACRO := MACRO_STMT [ ";" MACRO_STMT ]* */
static bool __parse_macro(struct mm_hid_macro_parser_s *p_start)
{
    struct mm_hid_macro_parser_s p = *p_start;

    while(1) {
        if(__parse_macro_stmt(&p)) {
            /* pass */
        } else {
            __REJECT();
        }

        if(__parse_token(&p, ";")) {
            /* pass */
        } else {
            break;
        }
    }

    if(*p.x == '\0') {
        /* pass */
    } else {
        __REJECT();
    }

__ACCEPT_LABEL:
    *p_start = p;
    __DEBUG_PRINT_ACCEPTED(p_start);
    return 1;
__REJECT_LABEL:
    __DEBUG_PRINT_REJECTED(p_start);
    return 0;
}

static inline void __reset_parser_config(struct mm_hid_macro_parser_config_s *c)
{
    memset(c, '\0', sizeof(struct mm_hid_macro_parser_config_s));
}

static inline void __reset_parser_data(struct mm_hid_macro_parser_data_s *d)
{
    memset(d, '\0', sizeof(struct mm_hid_macro_parser_data_s));
}

/* Public functions */

mm_hid_macro_parser_t *mm_hid_macro_parser_create(const struct mm_hid_macro_parser_config_s *config)
{
    struct mm_hid_macro_parser_s *p = (struct mm_hid_macro_parser_s *) malloc(sizeof(struct mm_hid_macro_parser_s));
    MM_CHECK_MALLOC_E(p);
    struct mm_hid_macro_parser_data_s *d = (struct mm_hid_macro_parser_data_s *) malloc(sizeof(struct mm_hid_macro_parser_data_s));
    MM_CHECK_MALLOC_E(d);
    struct mm_hid_macro_parser_config_s *c = (struct mm_hid_macro_parser_config_s *) malloc(sizeof(struct mm_hid_macro_parser_config_s));
    MM_CHECK_MALLOC_E(c);

    p->x = NULL;
    p->d = d;
    p->c = c;

    memcpy(p->c, config, sizeof(struct mm_hid_macro_parser_config_s));

    return p;
}

void mm_hid_macro_parser_destroy(mm_hid_macro_parser_t *p)
{
    free(p->d);
    free(p->c);
    free(p);
}

int mm_hid_macro_parse(mm_hid_macro_parser_t *p, const char *s)
{
    p->x = s;
    __reset_parser_data(p->d);
    return __parse_macro(p);
}

int mm_hid_macro_test(const char *s)
{
    int ret;

    static struct mm_hid_macro_parser_data_s d = {};
    static struct mm_hid_macro_parser_config_s c = {};

    static struct mm_hid_macro_parser_s p = {
        .x = NULL,
        .c = &c,
        .d = &d
    };

    p.x = NULL;
    __reset_parser_config(p.c);

    ret = mm_hid_macro_parse(&p, s);

    return ret;
}
