
#ifndef __MM_GENERIC_BACKLIGHT_H
#define __MM_GENERIC_BACKLIGHT_H

#include <stdint.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

mm_err_t mm_generic_backlight_init(void);
mm_err_t mm_generic_backlight_set(uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* __MM_GENERIC_BACKLIGHT_H */
