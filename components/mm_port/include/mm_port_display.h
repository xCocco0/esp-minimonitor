
#ifndef __MM_PORT_DISPLAY_H
#define __MM_PORT_DISPLAY_H

#include <stdint.h>
#include "mm_err.h"

#if LVGL_VERSION_MAJOR >= 9
#define MM_PORT_LV_ROTATION(deg) (deg == 90 ? LV_DISPLAY_ROTATION_90 : \
                                 (deg == 180 ? LV_DISPLAY_ROTATION_180 : \
                                 (deg == 270 ? LV_DISPLAY_ROTATION_270 : LV_DISPLAY_ROTATION_0)))
#else
#define MM_PORT_LV_ROTATION(deg) (deg == 90 ? LV_DISP_ROT_90 : \
                                 (deg == 180 ? LV_DISP_ROT_180 : \
                                 (deg == 270 ? LV_DISP_ROT_270 : LV_DISP_ROT_NONE)))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

mm_err_t mm_port_display_init(void *arg);

mm_err_t mm_port_display_set_bk(uint8_t level);
mm_err_t mm_port_display_rotate(int degrees);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PORT_DISPLAY_H */
