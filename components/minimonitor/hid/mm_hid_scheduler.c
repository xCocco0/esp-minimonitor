/**
 * @file
 *
 * @brief Schedule HID commands
 *
 * @note
 * This runs on a separate task rather than on the mm_event task to
 * reduce latency when sending HID commands. If in future we decide
 * to implement a priority queue in the mm_event queue then we can
 * just merge the two tasks and save some resources.
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_hid_scheduler.h"
#include "sdkconfig.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "mm_list.h"
#include "mm_macros.h"
#include "mm_log.h"
#include "mm_hid_macro.h"

/* randomize jitter as a triangular distribution */
#define __JITTER_TRIANGULAR_DIST 1

/* randomize jitter as a uniform distribution */
#define __JITTER_UNIFORM_DIST 0

#define __PENDING 0x1

static uint32_t s_last_id = 0;
static SemaphoreHandle_t s_list_sema = NULL;
static SemaphoreHandle_t s_parser_sema = NULL;
static EventGroupHandle_t s_events = NULL;
static MM_LIST_DEFINE(s_scheduler_list);
static mm_hid_macro_parser_t *s_parser = NULL;
static const struct timespec s_zero_timespec = {0,0};

struct mm_hid_scheduler_node_s {
    mm_list_node_t node;
    uint32_t tid; /**< Transaction ID that identifies the originating macro (not the node). */
    struct timespec timestamp;
    struct mm_hid_action_s action;
};

static void __parsed_macro_stmt_cb(struct mm_hid_macro_parser_s *p);
static void __parsed_wait_cb(struct mm_hid_macro_parser_s *p);
static void __parsed_key_cb(struct mm_hid_macro_parser_s *p);
static void __parsed_key_id_cb(struct mm_hid_macro_parser_s *p);

static const struct mm_hid_macro_parser_config_s s_parser_config = {
    .parsed_macro_stmt_cb = __parsed_macro_stmt_cb,
    .parsed_wait_cb = __parsed_wait_cb,
    .parsed_key_cb = __parsed_key_cb,
    .parsed_key_id_cb = __parsed_key_id_cb,
    .user_data = NULL
};

static struct parser_tmp_s {
    mm_key_t keys[CONFIG_MM_HID_MAX_NKRO];
    size_t keys_i;
    uint32_t start_t;
    int32_t magnitude;
    uint32_t duration_ms;
    uint32_t jitter_ms;
    uint32_t period_ms;
    uint32_t repeats_count; /**< the number of additional runs (e.g., 0 means run once) */
    int only_down : 1,
        only_up : 1;
} s_parser_tmp;

static void __insert_node(struct mm_hid_scheduler_node_s *n)
{
    /* sorted insert (key = timestamp) */
    MM_LIST_FOREACH(s_scheduler_list, it) {
        struct mm_hid_scheduler_node_s *n_i = MM_LIST_ITEM(it, struct mm_hid_scheduler_node_s, node);

        if(TIMESPEC_CMP(&n_i->timestamp, &n->timestamp) > 0) {
            MM_LIST_INSERT_BEFORE(s_scheduler_list, it, n->node);
            return;
        }
    }

    MM_LIST_ADD_BACK(s_scheduler_list, n->node);
}

int mm_hid_scheduler_cancel(uint32_t id)
{
    int ret = 0;

    xSemaphoreTake(s_list_sema, portMAX_DELAY);

    MM_LIST_FOREACH(s_scheduler_list, it) {
        struct mm_hid_scheduler_node_s *n_i = MM_LIST_ITEM(it, struct mm_hid_scheduler_node_s, node);

        if(n_i->tid == id) {
            MM_LIST_REMOVE(s_scheduler_list, it);
            ret++;
            free(n_i);
        }
    }
    xSemaphoreGive(s_list_sema);

    return ret;
}

static inline uint32_t new_id(void)
{
    if(s_last_id == UINT32_MAX) {
        s_last_id = 1;
    } else {
        s_last_id++;
    }
    return s_last_id;
}

/* parser callbacks */

/* KEY_ID := "A" | "B" | ... */
static void __parsed_key_id_cb(struct mm_hid_macro_parser_s *p)
{
    mm_key_t k = p->d->match.key_id;
    s_parser_tmp.keys[s_parser_tmp.keys_i++] = k;
}

/* KEY := KEY_ID [ "(" NUM ")" ] */
static void __parsed_key_cb(struct mm_hid_macro_parser_s *p)
{
    int32_t magnitude = p->d->match.key;
    s_parser_tmp.magnitude = magnitude;
}

/* WAIT := "Wait" [ "~" TIMEDELTA ] */
static void __parsed_wait_cb(struct mm_hid_macro_parser_s *p)
{
    uint32_t duration_ms = p->d->match.wait;
    s_parser_tmp.start_t += duration_ms;
}

static inline void __insert_key_down(mm_key_t key,
                                            const struct timespec* t_start,
                                            int32_t magnitude,
                                            uint32_t repeats_count,
                                            uint32_t period_ms,
                                            uint32_t jitter_ms)
{
    struct mm_hid_scheduler_node_s *n;

    n = (struct mm_hid_scheduler_node_s *) calloc(1, sizeof(struct mm_hid_scheduler_node_s));
    MM_CHECK_MALLOC_E(n);

    n->tid = s_last_id;
    n->timestamp = *t_start;
    n->action.key = key;
    n->action.key_down = 1;
    n->action.magnitude = magnitude;
    n->action.repeat.times = repeats_count;
    n->action.repeat.delay = period_ms;
    n->action.repeat.jitter = jitter_ms;

    __insert_node(n);
}

static inline void __insert_key_up(mm_key_t key,
                                            const struct timespec* t_start,
                                            int32_t magnitude,
                                            uint32_t repeats_count,
                                            uint32_t period_ms,
                                            uint32_t jitter_ms)
{
    struct mm_hid_scheduler_node_s *n;

    n = (struct mm_hid_scheduler_node_s *) calloc(1, sizeof(struct mm_hid_scheduler_node_s));
    MM_CHECK_MALLOC_E(n);

    n->tid = s_last_id;
    n->timestamp = *t_start;
    n->action.key = key;
    n->action.key_down = 0;
    n->action.magnitude = magnitude;
    n->action.repeat.times = repeats_count;
    n->action.repeat.delay = period_ms;
    n->action.repeat.jitter = jitter_ms;

    __insert_node(n);
}

static inline void __insert_mouse_cursor(mm_key_t key,
                                            struct timespec* t_start,
                                            int32_t magnitude,
                                            uint32_t duration_ms,
                                            uint32_t repeats_count,
                                            uint32_t period_ms,
                                            uint32_t jitter_ms)
{
    const uint32_t abs_magnitude = ABS(magnitude);
    int rounds;
    #define __SPLIT_BATCH_MS       CONFIG_MM_HID_MOUSE_XY_SPLIT_BATCH_MS
    #define __SPLIT_THRESHOLD_MS   CONFIG_MM_HID_MOUSE_XY_SPLIT_THRESHOLD_MS
    #define __SPLIT_MIN_SIZE       CONFIG_MM_HID_MOUSE_XY_SPLIT_SIZE_MIN

    /* compute the number of rounds.
       contraints:
       - must fit in the descriptor size: `|magnitude| / rounds < 128`
       - don't fragment into tiny chunks: `|magnitude| / rounds >= __SPLIT_MIN_SIZE`
       objectives:
       - `duration / rounds` close to __SPLIT_BATCH_MS
       - `duration / rounds` close to 128

       algorithm in a nutshell:
       - take the mean value of the 2 objectives:
       ```
                          duration            magnitude
                 ------------------------- + -----------
                  __SPLIT_BATCH_MS        128
       rounds = -----------------------------------------
                                   2
       ```
       - discard non-positive rounds
       - clamp in the interval `[|magnitude|/128, |magnitude|/__SPLIT_MIN_SIZE]`
    */

    if(duration_ms <= __SPLIT_THRESHOLD_MS) {
        rounds = 1;
        duration_ms = 0;
    } else {
        rounds = 1 + duration_ms/__SPLIT_BATCH_MS;
    }

    if(abs_magnitude / rounds > 127) {
        /* chunks are too large */
        rounds = CEIL_DIV(ABS(abs_magnitude), 127);
    } else if(abs_magnitude / rounds < __SPLIT_MIN_SIZE) {
        /* chunks are too small */
        rounds = abs_magnitude > __SPLIT_MIN_SIZE ? abs_magnitude/__SPLIT_MIN_SIZE : 1;
    }

    __insert_key_down(key, t_start, magnitude/rounds, repeats_count, period_ms, jitter_ms);
    magnitude -= magnitude/rounds;

    for(int r = rounds - 1; r > 0; --r) {

        TIMESPEC_ADD_MS(t_start, t_start, duration_ms/r);
        duration_ms -= duration_ms/r;

        __insert_key_down(key, t_start, magnitude/r, repeats_count, period_ms, jitter_ms);
        magnitude -= magnitude/r;
    }
}

static inline void __insert_reset(void)
{
    xSemaphoreTake(s_list_sema, portMAX_DELAY);

    __insert_key_up((mm_key_t) 0, &s_zero_timespec, 0, 0, 0, 0);

    xEventGroupSetBits(s_events, __PENDING);
    xSemaphoreGive(s_list_sema);
}

/* MACRO_STMT := ( KEY_COMBO [ "~" TIMEDELTA ] | WAIT ) */
static void __parsed_macro_stmt_cb(struct mm_hid_macro_parser_s *p)
{
    uint32_t duration_ms = p->d->match.macro_stmt;
    struct timespec now, start;

    if(s_parser_tmp.period_ms) {
        s_parser_tmp.duration_ms = MIN(duration_ms, s_parser_tmp.period_ms);
    } else {
        s_parser_tmp.duration_ms = duration_ms;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    TIMESPEC_ADD_MS(&start, &now, s_parser_tmp.start_t);

    xSemaphoreTake(s_list_sema, portMAX_DELAY);

    for(int i = 0; i < s_parser_tmp.keys_i; ++i) {

        if(!s_parser_tmp.only_up) {

            /* mouse movements are split into multiple reports over time. this has the advantage of:
             * - creating the illusion of time if the user specifies a duration,
             * - move the cursor more than 127 units at a time. */
            if(MM_KEY_IS_MOUSE_CURSOR(s_parser_tmp.keys[i])) {
                __insert_mouse_cursor(s_parser_tmp.keys[i], &start, s_parser_tmp.magnitude, s_parser_tmp.duration_ms,
                        s_parser_tmp.repeats_count, s_parser_tmp.period_ms, s_parser_tmp.jitter_ms);
            } else {
                __insert_key_down(s_parser_tmp.keys[i], &start, s_parser_tmp.magnitude,
                        s_parser_tmp.repeats_count, s_parser_tmp.period_ms, s_parser_tmp.jitter_ms);
            }
        }

        if(!s_parser_tmp.only_down) {

            if(MM_KEY_IS_MOUSE_CURSOR(s_parser_tmp.keys[i])) {
                /* key up does not exist */
            } else {
                TIMESPEC_ADD_MS(&start, &start, s_parser_tmp.duration_ms);
                __insert_key_up(s_parser_tmp.keys[i], &start, s_parser_tmp.magnitude,
                        s_parser_tmp.repeats_count, s_parser_tmp.period_ms, s_parser_tmp.jitter_ms);
            }
        }
    }
    xEventGroupSetBits(s_events, __PENDING);
    xSemaphoreGive(s_list_sema);

    /* clean up this statement modification */
    s_parser_tmp.keys_i = 0;
    s_parser_tmp.magnitude = 0;
    s_parser_tmp.duration_ms = CONFIG_MM_HID_DEFAULT_PRESS_DURATION_MS;
}

static uint32_t __scheduler_queue_common(const char *macro)
{
    if(s_parser == NULL) {
        /* we keep just 1 instance and reuse it every time */
        s_parser = mm_hid_macro_parser_create(&s_parser_config);
    }

    if(!macro) {
        return 0;
    }

    s_last_id = new_id();

    if(mm_hid_macro_parse(s_parser, macro)) {
        return s_last_id;
    } else {
        /* we could not parse the macro, we need to roll back */
        MM_LOG_W("Could not parse macro '%s'", macro);
        mm_hid_scheduler_cancel(s_last_id);

        __insert_reset();
        return 0;
    }
}

uint32_t mm_hid_scheduler_queue_periodic(const char *macro, uint32_t repeats_count, uint32_t period_ms, uint32_t jitter_ms)
{
    uint32_t ret;

    xSemaphoreTake(s_parser_sema, portMAX_DELAY);

    memset(&s_parser_tmp, '\0', sizeof(s_parser_tmp));
    s_parser_tmp.jitter_ms = jitter_ms;
    s_parser_tmp.duration_ms = CONFIG_MM_HID_DEFAULT_PRESS_DURATION_MS;
    s_parser_tmp.period_ms = MAX(1, period_ms);
    s_parser_tmp.repeats_count = repeats_count;

    ret = __scheduler_queue_common(macro);

    xSemaphoreGive(s_parser_sema);

    return ret;
}

uint32_t mm_hid_scheduler_queue_macro(const char *macro)
{
    uint32_t ret;

    xSemaphoreTake(s_parser_sema, portMAX_DELAY);

    memset(&s_parser_tmp, '\0', sizeof(s_parser_tmp));
    s_parser_tmp.duration_ms = CONFIG_MM_HID_DEFAULT_PRESS_DURATION_MS;

    ret = __scheduler_queue_common(macro);

    xSemaphoreGive(s_parser_sema);

    return ret;
}

uint32_t mm_hid_scheduler_queue_down(const char *macro)
{
    uint32_t ret;

    xSemaphoreTake(s_parser_sema, portMAX_DELAY);

    memset(&s_parser_tmp, '\0', sizeof(s_parser_tmp));
    s_parser_tmp.only_down = 1;

    ret = __scheduler_queue_common(macro);

    xSemaphoreGive(s_parser_sema);

    return ret;
}

uint32_t mm_hid_scheduler_queue_up(const char *macro)
{
    uint32_t ret;

    xSemaphoreTake(s_parser_sema, portMAX_DELAY);

    memset(&s_parser_tmp, '\0', sizeof(s_parser_tmp));
    s_parser_tmp.only_up = 1;
    s_parser_tmp.duration_ms = 0;

    ret = __scheduler_queue_common(macro);

    xSemaphoreGive(s_parser_sema);

    return ret;
}

uint32_t mm_hid_scheduler_queue_raw(mm_key_t key, int key_down, int16_t magnitude)
{
    struct timespec now;
    uint32_t id;

    clock_gettime(CLOCK_MONOTONIC, &now);

    xSemaphoreTake(s_list_sema, portMAX_DELAY);

    id = new_id();

    if(MM_KEY_IS_MOUSE_CURSOR(key)) {
        __insert_mouse_cursor(key, &now, (int32_t) magnitude, 0, 0, 0, 0);
    } else if(key_down) {
        __insert_key_down(key, &now, (int32_t) magnitude, 0, 0, 0);
    } else {
        __insert_key_up(key, &now, (int32_t) magnitude, 0, 0, 0);
    }

    xEventGroupSetBits(s_events, __PENDING);
    xSemaphoreGive(s_list_sema);

    return id;
}

static uint32_t __murmur_mix(uint32_t h)
{
    /* Final bit mixing from https://en.wikipedia.org/wiki/MurmurHash */
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

static uint32_t __add_jitter(uint32_t base_ms, uint32_t jitter_ms, uint32_t seed)
{
    uint32_t rnd;
    int32_t signed_rnd = 0, variation = 0;

    if(jitter_ms > 0) {
        rnd = __murmur_mix(seed);

        #if __JITTER_TRIANGULAR_DIST
        /* Triangular distribution */
        signed_rnd = (int16_t)(rnd & UINT8_MAX) - (int16_t)((rnd >> 8) & UINT8_MAX); /* [-255, 255] */
        variation = ((signed_rnd * (int32_t) jitter_ms) / UINT8_MAX);
        #else
        /* Uniform distribution */
        (void) signed_rnd;
        variation = (int32_t) (rnd % (2 * jitter_ms)) - (int32_t) jitter_ms;
        #endif
    }

    /* avoid underflow */
    if(variation < 0 && -variation >= base_ms) {
        return base_ms;
    }

    return base_ms + variation;
}

void mm_hid_scheduler_init(void)
{
    s_list_sema = xSemaphoreCreateMutex();
    MM_CHECK_E(s_list_sema != NULL);

    s_parser_sema = xSemaphoreCreateMutex();
    MM_CHECK_E(s_parser_sema != NULL);

    s_events = xEventGroupCreate();
    MM_CHECK_E(s_events != NULL);
}

int __schedule_next(struct mm_hid_action_s *next, uint32_t *timeout_ms)
{
    struct timespec now, t_to_next;
    struct mm_hid_scheduler_node_s *n = NULL;
    uint32_t t_to_next_ms;
    uint32_t seed;
    mm_list_iterator_t head_it;
    int queue_waiting;
    int ret;

    clock_gettime(CLOCK_MONOTONIC, &now);

    xSemaphoreTake(s_list_sema, portMAX_DELAY);

    queue_waiting = MM_LIST_LEN(s_scheduler_list);
    if(queue_waiting > 0) {

        /* calculate time until next event */
        head_it = MM_LIST_BEGIN(s_scheduler_list);
        n = MM_LIST_ITEM(head_it, struct mm_hid_scheduler_node_s, node);

        if(TIMESPEC_CMP(&n->timestamp, &now) <= 0) {
            /* we can schedule this node */

            memcpy(next, &n->action, sizeof(struct mm_hid_action_s));
            ret = 1;

            MM_LIST_REMOVE_FRONT(s_scheduler_list);

            /* schedule again later */
            if(n->action.repeat.times > 0) {
                n->action.repeat.times--;
                /* TODO: we use the seed to add the same delay to down/up events (because the press
                *       time should be contant regardless of the jitter). this is ugly and we should
                *       really just schedule the UP event after each DOWN event in future.
                */
                seed = (1 + n->tid) * (1 + n->action.repeat.times);
                TIMESPEC_ADD_MS(&n->timestamp, &n->timestamp, __add_jitter(n->action.repeat.delay, n->action.repeat.jitter, seed));
                __insert_node(n);
            } else {
                free(n);
            }
        } else {
            /* not ready yet */
            ret = 0;
        }
    } else {
        /* nothing to schedule */
        ret = 0;
    }

    /* compute time to next */
    queue_waiting = MM_LIST_LEN(s_scheduler_list);
    if(queue_waiting) {
        /* get next in queue */
        head_it = MM_LIST_BEGIN(s_scheduler_list);
        n = MM_LIST_ITEM(head_it, struct mm_hid_scheduler_node_s, node);

        if(TIMESPEC_CMP(&n->timestamp, &now) >= 0) {
            TIMESPEC_DIFF(&t_to_next, &n->timestamp, &now);
            t_to_next_ms = TIMESPEC_TO_MS(&t_to_next);
        } else {
            /* next is ready */
            t_to_next_ms = 0;
        }
    } else {
        /* queue empty */
        t_to_next_ms = UINT32_MAX;
    }

    xSemaphoreGive(s_list_sema);

    if(timeout_ms) {
        *timeout_ms = t_to_next_ms;
    }

    return ret;
}

int mm_hid_scheduler_schedule_next(struct mm_hid_action_s *ev, uint32_t *timeout_ms)
{
    MM_CHECK_E(ev);
    /* if we receive a pending event continue, otherwise wait for the time given as argument */
    /* note: this is not the most correct choice, because freertos waits AT
       MOST timeout_ms (depending on the tick duration), and would make the
       task busy-wait, wasting CPU, just before a key should trigger. */
    if(timeout_ms) {
        if(xEventGroupWaitBits(s_events, __PENDING, pdTRUE, pdFALSE, pdMS_TO_TICKS(*timeout_ms))) {
            /* someone queued an event */
        }
    }
    return __schedule_next(ev, timeout_ms);
}

int mm_hid_scheduler_in_queue(void)
{
    int ret;

    xSemaphoreTake(s_list_sema, portMAX_DELAY);

    ret = MM_LIST_LEN(s_scheduler_list);

    xSemaphoreGive(s_list_sema);

    return ret;
}
