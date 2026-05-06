/**
 * @file
 *
 * @brief Minimonitor errors and logging
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_err.h"

uint32_t mm_err;
uint32_t mm_err_at_startup = 0;

uint32_t mm_err_clear(void)
{
    uint32_t ret = mm_err;
    mm_err = 0;
    return ret;
}
