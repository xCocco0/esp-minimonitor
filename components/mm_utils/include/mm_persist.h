/**
 * @file
 *
 * @brief Utilities for handling object persistency into NVS
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_PERSIST_H
#define __MM_PERSIST_H

#include <stddef.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mm_persist_init(void);

mm_err_t mm_persist_store_obj(const char *key, const void *buf, size_t size);
mm_err_t mm_persist_fetch_obj(const char *key, void *buf, size_t *size);
mm_err_t mm_persist_invalidate_obj(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PERSIST_H */
