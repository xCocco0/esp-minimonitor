/**
 * @file
 *
 * @brief Handle buttons and other IO peripherals
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_IO_H
#define __MM_IO_H

#ifdef __cplusplus
extern "C"
{
#endif

void mm_io_init(void);
int mm_io_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_IO_H */
