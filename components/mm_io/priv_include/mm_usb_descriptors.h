/**
 * @file
 *
 * @brief USB descriptors
 *
 * @note This file does not replace the espressif wrappers but we need some more
 * customization that is not possible through the Kconfig interface.
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_USB_DESCRIPTORS_H
#define __MM_USB_DESCRIPTORS_H

#include "sdkconfig.h"

#include <stdint.h>
#include "tinyusb.h"

#define TTY_CDC_NUM TINYUSB_CDC_ACM_0

/* These checks guarantee that our Kconfig entries are in line with TinyUSB ones.
   This is stupid but I don't want to rely on the Kconfig tools. */

#if CONFIG_MM_IO_ENABLE_USB_OTG

#if !(CONFIG_TINYUSB_CDC_COUNT > 0)
#error Set CONFIG_TINYUSB_CDC_COUNT to 1.
#endif

#if CONFIG_MM_IO_ENABLE_USB_MSC ^ CONFIG_TINYUSB_MSC_ENABLED
#error If MM_IO_ENABLE_USB_MSC is enabled, you need to enable TINYUSB_MSC_ENABLED too.
#endif

#if CONFIG_MM_IO_ENABLE_USB_HID ^ (CONFIG_TINYUSB_HID_COUNT > 0)
#error If MM_IO_ENABLE_USB_HID is enabled, you need to set TINYUSB_HID_COUNT to 1.
#endif

#endif

#if CONFIG_TINYUSB_NET_MODE_NCM || (CONFIG_TINYUSB_VENDOR_COUNT > 0)
#warning NCM or Vendor enabled in Kconfig, please disable them.
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* ITF numbers */
enum {
    #if CFG_TUD_CDC
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    #endif
    #if CFG_TUD_CDC > 1
    ITF_NUM_CDC1,
    ITF_NUM_CDC1_DATA,
    #endif
    #if CFG_TUD_MSC
    ITF_NUM_MSC,
    #endif
    #if CFG_TUD_HID
    ITF_NUM_HID,
    #endif
    ITF_NUM_TOTAL
};

/* Endpoints */
enum {
    // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
    EP_EMPTY = 0,
    #if CFG_TUD_CDC
    EPNUM_0_CDC_NOTIF,
    EPNUM_0_CDC,
    #endif
    #if CFG_TUD_CDC > 1
    EPNUM_1_CDC_NOTIF,
    EPNUM_1_CDC,
    #endif
    #if CFG_TUD_MSC
    EPNUM_MSC,
    #endif
    #if CFG_TUD_HID
    EPNUM_HID,
    #endif
};

/* Indices into string descriptor */
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    #if CFG_TUD_CDC
    STRID_CDC_INTERFACE,
    #endif
    #if CFG_TUD_MSC
    STRID_MSC_INTERFACE,
    #endif
    #if CFG_TUD_HID
    STRID_HID_INTERFACE,
    #endif
};

/* HID report IDs */
enum {
  REPORT_ID_KEYBOARD = 1,
  REPORT_ID_MOUSE,
  REPORT_ID_CONSUMER
};

enum {
    TUSB_DESC_TOTAL_LEN = TUD_CONFIG_DESC_LEN +
                          CFG_TUD_CDC * TUD_CDC_DESC_LEN +
                          CFG_TUD_MSC * TUD_MSC_DESC_LEN +
                          CFG_TUD_HID * TUD_HID_DESC_LEN +
                          CFG_TUD_NCM * TUD_CDC_NCM_DESC_LEN +
                          CFG_TUD_VENDOR * TUD_VENDOR_DESC_LEN
};

static char serial_str[18];

static const char *string_descriptor[] = {
    (char[]){0x09, 0x04},                         /* Supported language is English (0x0409) */
    "Espressif",                                  /* Manufacturer */
    "MiniMonitor",                                /* Product */
    serial_str,                                   /* Serials, should use chip ID */
    #if CFG_TUD_CDC
    [STRID_CDC_INTERFACE] = "MiniMonitor CDC",    /* CDC */
    #endif
    #if CFG_TUD_MSC
    [STRID_MSC_INTERFACE] = "MiniMonitor MSC",    /* Mass storage device */
    #endif
    #if CFG_TUD_HID
    [STRID_HID_INTERFACE] = "MiniMonitor HID",    /* Human interface device */
    #endif
};

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER))
};

static const uint8_t descriptor_fs_cfg[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    #if CFG_TUD_CDC
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_CDC_INTERFACE, 0x80 | EPNUM_0_CDC_NOTIF, 8, EPNUM_0_CDC, 0x80 | EPNUM_0_CDC, 64),
    #endif
    #if CFG_TUD_MSC
    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, STRID_MSC_INTERFACE, EPNUM_MSC, 0x80 | EPNUM_MSC, 64),
    #endif
    #if CFG_TUD_HID
    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, STRID_HID_INTERFACE, false, sizeof(hid_report_descriptor), 0x80 | EPNUM_HID, 16, 10),
    #endif
};

#if (TUD_OPT_HIGH_SPEED)
uint8_t const descriptor_hs_cfg_default[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    #if CFG_TUD_CDC
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_CDC_INTERFACE, 0x80 | EPNUM_0_CDC_NOTIF, 8, EPNUM_0_CDC, 0x80 | EPNUM_0_CDC, 512),
    #endif
    #if CFG_TUD_MSC
    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, STRID_MSC_INTERFACE, EPNUM_MSC, 0x80 | EPNUM_MSC, 512),
    #endif
    #if CFG_TUD_HID
    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, STRID_HID_INTERFACE, false, sizeof(hid_report_descriptor), 0x80 | EPNUM_HID, 16, 512),
    #endif
};
#endif // TUD_OPT_HIGH_SPEED

#ifdef __cplusplus
}
#endif

#endif /* __MM_USB_DESCRIPTORS_H */
