/**
 * @file
 *
 * @brief Handle buttons and other IO peripherals
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_io.h"

#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_err.h"
#include "mm_port.h"
#include "mm_io_button.h"
#include "mm_io_ledstrip.h"
#include "mm_io_imu.h"
#include "mm_io_usb.h"

static int s_initialized = 0;

void mm_io_init(void)
{
    MM_CHECK_E(s_initialized == 0);
    MM_CHECK_E(mm_conf_initialized());
    MM_CHECK_E(mm_port_initialized());
    MM_CHECK_E(mm_initialized());

    mm_io_button_init();
    mm_io_ledstrip_init();
    #if CONFIG_MM_PORT_HAS_IMU
    mm_io_imu_init();
    mm_io_imu_detect_rotation_start();
    #endif

    #if CONFIG_MM_IO_ENABLE_USB_OTG
    mm_io_usb_init();
    #endif

    s_initialized = 1;
    MM_LOG_I("IO initialized");
}

int mm_io_initialized(void)
{
    return s_initialized;
}
