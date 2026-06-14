/**
 * @file
 *
 * @brief Minimonitor errors and logging
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_log.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "mm_err.h"
#include "mm_ringbuf.h"
#include "mm_macros.h"

static int s_initialized = 0;
static char s_line_buf[CONFIG_MM_LOG_MAX_LINE_LENGTH];
static int s_paused = 0;
static size_t s_pause_index = 0;
static char EXT_RAM_NOINIT_ATTR s_log_buf[CONFIG_MM_LOG_HISTORY_BYTES+1];
static __NOINIT_ATTR mm_ringbuf_t s_log_rb; // = MM_RINGBUF_INIT(s_log_buf, CONFIG_MM_LOG_HISTORY_BYTES);

void mm_log_init(void)
{
    MM_CHECK_E(s_initialized == 0);

    /* since the ring buffer has noinit double check that the member values make sense */
    s_log_rb.buf = s_log_buf;
    s_log_rb.bufsize = CONFIG_MM_LOG_HISTORY_BYTES;
    if(s_log_rb.end >= CONFIG_MM_LOG_HISTORY_BYTES) {
        s_log_rb.end = 0;
    }
    /* start empty */
    s_log_rb.begin = s_log_rb.end;

    s_log_buf[sizeof(s_log_buf)-1] = '\0';

    printf("b %u, e %u\n", s_log_rb.begin, s_log_rb.end);
    printf("buffer:\n---\n%s\n---\n", s_log_rb.buf);

    esp_log_set_vprintf(mm_log_vprintf);

    s_initialized = 1;
    MM_LOG_I("Log initialized");
}

int mm_log_vprintf(const char *fmt, va_list va)
{
    int ret;
    static _Thread_local int reent = 0;

    if(reent > 0) {
        return -1;
    } else {
        ++reent;
    }

    ret = vsnprintf(s_line_buf, sizeof(s_line_buf), fmt, va);

    if(ret > 0) {
        MM_RINGBUF_WRITE_BACK(&s_log_rb, s_line_buf, ret);
        if(!s_paused) {
            ret = printf("%s", s_line_buf);
        }
    }

    --reent;

    return ret;
}

void mm_log(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    mm_log_vprintf(format, list);
    va_end(list);
}

void mm_log_pause(void)
{
    s_pause_index = s_log_rb.end;
    s_paused = 1;
}

void mm_log_resume(void)
{
    size_t i = s_pause_index;

    /* print buffered data */
    while(i != s_log_rb.end) {
        putchar(s_log_buf[i]);
        i = (i + 1) % CONFIG_MM_LOG_HISTORY_BYTES;
    }

    s_paused = 0;
}

char *mm_log_get_history(char **part1, size_t *part1_len, char **part2, size_t *part2_len)
{
    if(part1) {
        *part1 = s_log_buf + s_log_rb.end;
    }
    if(part1_len) {
        *part1_len = CONFIG_MM_LOG_HISTORY_BYTES - s_log_rb.end;
    }
    if(part2) {
        *part2 = s_log_buf;
    }
    if(part2_len) {
        *part2_len = s_log_rb.end;
    }
    return s_log_buf + s_log_rb.begin;
}

void mm_log_print_history(void)
{
    char *part1, *part2;
    char tmp;
    mm_log_get_history(&part1, NULL, &part2, NULL);
    tmp = *part2;
    *part2 = '\0';
    printf("%s%s\n", part1, part2+1);
    *part2 = tmp;
}
