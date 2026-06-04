
#include "mm_port.h"

#include "mm_err.h"
#include <lvgl.h>

static int s_initialized_default = 0;

void __weak_symbol mm_port_init(void)
{
    MM_CHECK_E(mm_port_display_init(NULL) == MM_OK);

    MM_CHECK_E(mm_port_imu_init(NULL) == MM_OK);

    /* if it doesn't work we don't care, images will just not
     * show up */
    MM_CHECK_W(mm_port_fs_init(NULL) == MM_OK);

    s_initialized_default = 1;
}

int mm_port_initialized(void)
{
    return s_initialized_default;
}

mm_err_t __weak_symbol mm_port_display_init(void *arg)
{
    (void) arg;
    return MM_FAIL;
}

mm_err_t __weak_symbol mm_port_display_set_bk(uint8_t level)
{
    (void) level;
    return MM_FAIL;
}

mm_err_t __weak_symbol mm_port_display_rotate(int degrees)
{
    (void) degrees;
    return MM_FAIL;
}

mm_err_t __weak_symbol mm_port_imu_init(void *arg)
{
    (void) arg;
    #if CONFIG_MM_PORT_HAS_IMU
    return MM_FAIL;
    #else
    return MM_OK;
    #endif
}

mm_err_t __weak_symbol mm_port_imu_read_gyro(int16_t *x, int16_t *y, int16_t *z)
{
    return MM_FAIL;
}

mm_err_t __weak_symbol mm_port_imu_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    return MM_FAIL;
}
mm_err_t __weak_symbol mm_port_fs_init(void *arg)
{
    #if CONFIG_MM_PORT_HAS_SDCARD
    return MM_FAIL;
    #else
    return MM_OK;
    #endif
}

int __weak_symbol mm_port_fs_available(void)
{
    return 0;
}

void __weak_symbol *mm_port_fs_get_iohandle(void)
{
    return NULL;
}

