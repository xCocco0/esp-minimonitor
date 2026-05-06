
#ifndef __MM_GENERIC_BACKLIGHT_CONFIG_H
#define __MM_GENERIC_BACKLIGHT_CONFIG_H

#define MM_PORT_BL_GPIO              22
#define MM_PORT_BL_LS_MODE           LEDC_LOW_SPEED_MODE
#define MM_PORT_BL_TEST_DUTY         (4000)
#define MM_PORT_BL_ResolutionRatio   LEDC_TIMER_13_BIT
#define MM_PORT_BL_MAX_Duty          ((1 << MM_PORT_BL_ResolutionRatio) - 1)
#define MM_PORT_BL_HS_TIMER          LEDC_TIMER_0
#define MM_PORT_BL_HS_CHANNEL        LEDC_CHANNEL_0

#endif
