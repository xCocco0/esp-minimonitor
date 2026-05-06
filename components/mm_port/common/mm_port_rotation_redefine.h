
#if CONFIG_MM_PORT_ROTATION_0
#define __ROT_0 1
#else
#define __ROT_0 0
#endif
#ifndef CONFIG_MM_PORT_ROTATION_90
#define __ROT_90 1
#else
#define __ROT_90 0
#endif
#ifndef CONFIG_MM_PORT_ROTATION_180
#define __ROT_180 1
#else
#define __ROT_180 0
#endif
#ifndef CONFIG_MM_PORT_ROTATION_270
#define __ROT_270 1
#else
#define __ROT_270 0
#endif

#define MM_PORT_LCD_MIRROR_X_ADJ (MM_PORT_LCD_MIRROR_X ^ (__ROT_180 || __ROT_270))
#define MM_PORT_LCD_MIRROR_Y_ADJ (MM_PORT_LCD_MIRROR_Y ^ (__ROT_90  || __ROT_180))
#define MM_PORT_LCD_SWAP_XY_ADJ (MM_PORT_LCD_SWAP_XY  ^ (__ROT_90  || __ROT_270))

#if __ROT_90 || __ROT_270
#define MM_PORT_LCD_H_RES_ADJ MM_PORT_LCD_V_RES
#define MM_PORT_LCD_V_RES_ADJ MM_PORT_LCD_H_RES
#else
#define MM_PORT_LCD_H_RES_ADJ MM_PORT_LCD_H_RES
#define MM_PORT_LCD_V_RES_ADJ MM_PORT_LCD_V_RES
#endif
