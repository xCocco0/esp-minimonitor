/**
 * @file
 *
 * @brief Minimonitor errors and logging
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_log.h"
#include "mm_err.h"
#include "mm_macros.h"
#include "sdkconfig.h"

static int s_initialized = 0;
static int s_buf_index = 0;
static char __NOINIT_ATTR s_log_buf[CONFIG_MM_LOG_HISTORY_BYTES+CONFIG_MM_LOG_MAX_LINE_LENGTH];

/* e.g.: BUFS=20 LINES=15
0                  BUFS       BUFS+LINES
:                   :             :
+-------BUFS-------++----LINES----+
Lorem ipsum dolor sic amet0
^ index(before)           ^ index(after)
c amet0psum dolor si0 amet0
      ^ index
0 ametAdipiscit alit0
^ index
Very long string ab00
                   ^ index
*/

void mm_log_init(void)
{
    MM_CHECK_E(s_initialized == 0);
    esp_log_set_vprintf(mm_log_vprintf);

    s_initialized = 1;
    MM_LOG_I("Log initialized");
}

int mm_log_vprintf(const char *fmt, va_list va)
{
    int ret;

    ret = vsnprintf(&s_log_buf[s_buf_index], sizeof(s_log_buf) - s_buf_index, fmt, va);

    if(ret > 0) {
        s_buf_index += MIN(ret, sizeof(s_log_buf) - s_buf_index - 1);

        if(s_buf_index >= CONFIG_MM_LOG_HISTORY_BYTES) {
            memcpy(s_log_buf, &s_log_buf[CONFIG_MM_LOG_HISTORY_BYTES], s_buf_index - CONFIG_MM_LOG_HISTORY_BYTES + 1);
            s_log_buf[CONFIG_MM_LOG_HISTORY_BYTES] = '\0';
            s_buf_index %= CONFIG_MM_LOG_HISTORY_BYTES;
        }
    }

    return vprintf(fmt, va);
}

void mm_log(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    mm_log_vprintf(format, list);
    va_end(list);
}

char *mm_log_history_part1(void) {
    return &s_log_buf[s_buf_index+1];
}

char *mm_log_history_part2(void) {
    return s_log_buf;
}

void mm_log_print_history(void)
{
    MM_CHECK_E(s_initialized == 1);
    printf("%s%s\n", mm_log_history_part1(), mm_log_history_part2());
}
