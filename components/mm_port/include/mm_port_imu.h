
#ifndef __MM_PORT_IMU_H
#define __MM_PORT_IMU_H

#include <stdint.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

mm_err_t mm_port_imu_init(void *arg);

/** @note On x,y,z orientation with respect to the screen:
 *  - X-axis is on the screen (when rotation=0) horizontal length from left to right,
 *  - Y-axis is on the screen (when rotation=0) vertical length from bottom to top,
 *  - Z-axis is perpendicular to the screen pointing towards the observer.
 */

 /** @note On the IMU units:
 *  - We store a 16 bits integer where a value of 1 equals 4g/2^15.
 */

mm_err_t mm_port_imu_read_gyro(int16_t *x, int16_t *y, int16_t *z);
mm_err_t mm_port_imu_read_accel(int16_t *x, int16_t *y, int16_t *z);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PORT_IMU_H */
