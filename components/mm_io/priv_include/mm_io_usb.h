/**
 * @file
 *
 * @brief Handles USB as HID device
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_USBHID_H
#define __MM_USBHID_H

#ifdef __cplusplus
extern "C"
{
#endif

void mm_io_usb_init(void);
void mm_io_usb_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_USBHID_H */
