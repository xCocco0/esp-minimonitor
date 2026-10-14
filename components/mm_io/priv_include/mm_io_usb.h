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

#include "mm_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void mm_io_usb_init(void);
void mm_io_usb_deinit(void);

mm_err_t mm_usb_send_report_consumer(uint16_t control);
mm_err_t mm_usb_send_report_keyboard(uint8_t modifier, const uint8_t keycode[6]);
mm_err_t mm_usb_send_report_mouse(int is_absolute, uint8_t buttons, int16_t x, int16_t y, int16_t wheel, int16_t pad);

#ifdef __cplusplus
}
#endif

#endif /* __MM_USBHID_H */
