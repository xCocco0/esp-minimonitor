/**
 * @file
 *
 * @brief Minimonitor logging
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_LOG_H
#define __MM_LOG_H

#include "esp_rom_sys.h"
#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MM_LOG MM_LOG_D

#ifndef TAG
#define TAG "-"
#endif

#define MM_LOG_E(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define MM_LOG_W(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#define MM_LOG_I(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define MM_LOG_D(fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define MM_LOG_V(fmt, ...) ESP_LOGV(TAG, fmt, ##__VA_ARGS__)

void mm_log_init(void);
int mm_log_vprintf(const char *fmt, va_list va);
void mm_log(const char *format, ...);
void mm_log_print_history(void);
void mm_log_pause(void);
void mm_log_resume(void);
char *mm_log_get_history(char **part1, size_t *part1_len, char **part2, size_t *part2_len);

#ifdef __cplusplus
}
#endif

#endif /* __MM_LOG_H */
