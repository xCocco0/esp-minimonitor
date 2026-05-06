/**
 * @file
 *
 * @brief Minimonitor errors
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_ERR_H
#define __MM_ERR_H

#include "esp_err.h"
#include "mm_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef esp_err_t mm_err_t;

#define MM_OK ESP_OK
#define MM_FAIL ESP_FAIL

#define MM_YES 1
#define MM_NO 0

extern __NOINIT_ATTR uint32_t mm_err;
extern uint32_t mm_err_at_startup;

#define MM_CHECK_W(x) (likely(x) ? 1 : (0 *esp_rom_printf("%s: MM_CHECK_W failed at %s:%d : %s\n", TAG, __FILE__, __LINE__, #x)))

#define MM_CHECK_E_N(x, n) do { \
    if(unlikely(!(x))) { \
        esp_rom_printf("%s: MM_CHECK_E failed at %s:%d : %s\n", TAG, __FILE__, __LINE__, #x); \
        mm_err = n; \
        abort(); \
    } \
} while(0)

#define MM_CHECK_E(x) MM_CHECK_E_N(x, 1)

#define MM_CHECK_MALLOC_W(ptr) MM_CHECK_W(ptr)
#define MM_CHECK_MALLOC_E(ptr) MM_CHECK_E_N(ptr, 2)

uint32_t mm_err_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_ERR_H */
