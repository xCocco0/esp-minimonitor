
#ifndef __MM_PORT_BOARD_CONFIG_H
#define __MM_PORT_BOARD_CONFIG_H

#include "sdkconfig.h"

/* LCD config */

#define MM_PORT_LCD_H_RES CONFIG_MM_PORT_LCD_H_RES
#define MM_PORT_LCD_V_RES CONFIG_MM_PORT_LCD_V_RES

#define MM_PORT_LCD_OFFSET_X 0
#define MM_PORT_LCD_OFFSET_Y 0
#define MM_PORT_LCD_MIRROR_X false
#define MM_PORT_LCD_MIRROR_Y false
#define MM_PORT_LCD_SWAP_XY false
#define MM_PORT_LCD_INVERT_COLOR true

#include "mm_port_rotation_redefine.h"

#define MM_PORT_LCD_INTERFACE_SPI    1
#define MM_PORT_LCD_SPI_NUM          SPI2_HOST
#define MM_PORT_LCD_SPI_MODE         0
#define MM_PORT_LCD_SPI_TRANS_QUEUE  10

#define MM_PORT_LCD_GPIO_BL     1
#define MM_PORT_LCD_GPIO_DC     42
#define MM_PORT_LCD_GPIO_CS     45
#define MM_PORT_LCD_GPIO_RST    0
#define MM_PORT_LCD_GPIO_SCLK   39
#define MM_PORT_LCD_GPIO_MOSI   38
#define MM_PORT_LCD_GPIO_MISO   40
#define MM_PORT_LCD_TP_INT      46
#define MM_PORT_LCD_TP_SCL      47
#define MM_PORT_LCD_TP_SDA      48

#define MM_PORT_LCD_BL_ON_LEVEL            1
#define MM_PORT_LCD_DRAW_BUFF_DOUBLE       1
#define MM_PORT_LCD_DRAW_BUFF_HEIGHT       80
#define MM_PORT_LCD_BUFF_DMA         0
#define MM_PORT_LCD_BUFF_IN_SPIRAM   0
#define MM_PORT_LCD_USE_FULL_REFRESH 0

#define MM_PORT_LCD_CMD_BITS         8
#define MM_PORT_LCD_PARAM_BITS       8
#define MM_PORT_LCD_BITS_PER_PIXEL   16
#define MM_PORT_LCD_PIXEL_CLOCK_HZ   80 * 1000 * 1000

#define MM_PORT_LCD_DATA_ENDIANNESS LCD_RGB_DATA_ENDIAN_BIG
#define MM_PORT_LCD_RGB_ENDIANNESS  LCD_RGB_ELEMENT_ORDER_RGB

/* Backlight config */

#define MM_PORT_BL_GPIO              1
#define MM_PORT_BL_LS_MODE           LEDC_LOW_SPEED_MODE
#define MM_PORT_BL_TEST_DUTY         (4000)
#define MM_PORT_BL_ResolutionRatio   LEDC_TIMER_13_BIT
#define MM_PORT_BL_MAX_Duty          ((1 << MM_PORT_BL_ResolutionRatio) - 1)
#define MM_PORT_BL_HS_TIMER          LEDC_TIMER_0
#define MM_PORT_BL_HS_CHANNEL        LEDC_CHANNEL_0

/* SD card config */

#define MM_PORT_SDCARD_INTERFACE_SPI 1

#define MM_PORT_SDCARD_SPI_NUM    SPI2_HOST

#define MM_PORT_SDCARD_PIN_MOSI   38
#define MM_PORT_SDCARD_PIN_MISO   40
#define MM_PORT_SDCARD_PIN_CLK    39
#define MM_PORT_SDCARD_PIN_CS     41
#define MM_PORT_SDCARD_PIN_CD     -1
#define MM_PORT_SDCARD_PIN_WP     -1
#define MM_PORT_SDCARD_PIN_INT    -1

#endif /* __MM_PORT_BOARD_CONFIG_H */
