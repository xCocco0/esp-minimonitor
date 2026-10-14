
/**
 * @file
 *
 * @brief The task sending HID reports through USB
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_IO_USB_HID_H
#define __MM_IO_USB_HID_H

#ifdef __cplusplus
extern "C"
{
#endif

void mm_usb_hid_task_spawn(void);
void mm_usb_hid_task_kill(void);

void mm_usb_hid_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __MM_IO_USB_HID_H */
