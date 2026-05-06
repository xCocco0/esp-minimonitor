/**
 * @file
 *
 * @brief Handle gyroscope and acceleration sensor
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_IO_IMU_H
#define __MM_IO_IMU_H

#ifdef __cplusplus
extern "C"
{
#endif

void mm_io_imu_init(void);

void mm_io_imu_detect_rotation_start(void);
void mm_io_imu_detect_rotation_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_IO_IMU_H */
