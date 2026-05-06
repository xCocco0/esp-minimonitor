
#ifndef __MM_GENERIC_BACKLIGHT_CONFIG_H
#define __MM_GENERIC_BACKLIGHT_CONFIG_H

#include "sdkconfig.h"

#define MM_PORT_BL_GPIO              CONFIG_MM_PORT_LCD_GPIO_BL
#define MM_PORT_BL_LS_MODE           LEDC_LOW_SPEED_MODE
#define MM_PORT_BL_TEST_DUTY         (4000)
#define MM_PORT_BL_ResolutionRatio   LEDC_TIMER_13_BIT
#define MM_PORT_BL_MAX_Duty          ((1 << MM_PORT_BL_ResolutionRatio) - 1)
#define MM_PORT_BL_HS_TIMER          CONFIG_MM_PORT_LEDC_HS_TIMER
#define MM_PORT_BL_HS_CHANNEL        CONFIG_MM_PORT_LEDC_HS_CHANNEL

#endif
