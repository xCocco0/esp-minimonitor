
#include "mm_port_imu.h"

#include "bsp_qmi8658.h"

mm_err_t mm_port_imu_init(void *arg)
{
    (void) arg;
    bsp_qmi8658_init();
    return MM_OK;
}

static inline void __assign(int16_t *x, int16_t *y, int16_t *z, int16_t x_val, int16_t y_val, int16_t z_val)
{
    *x = x_val;
    *y = y_val;
    *z = z_val;
}

#define TRANSPLACE_ORIGIN_XYZ(x, y, z) x, -y, z

mm_err_t mm_port_imu_read_gyro(int16_t *x, int16_t *y, int16_t *z)
{
    bsp_qmi8658_read_gyro(x, y, z);
    __assign(x, y, z, TRANSPLACE_ORIGIN_XYZ(*x,*y,*z));
    return MM_OK;
}

mm_err_t mm_port_imu_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    bsp_qmi8658_read_accel(x, y, z);
    __assign(x, y, z, TRANSPLACE_ORIGIN_XYZ(*x,*y,*z));
    return MM_OK;
}
