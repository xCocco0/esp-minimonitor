
#ifndef __MM_GENERIC_DISPLAY_DRIVER_H
#define __MM_GENERIC_DISPLAY_DRIVER_H

#include "driver/ledc.h"
#include "esp_lcd_types.h"
#include "esp_lvgl_port.h"
#include "esp_err.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern esp_lcd_panel_handle_t mm_lcd_panel;
extern esp_lcd_panel_io_handle_t mm_lcd_io;
extern lv_display_t *mm_lvgl_disp;
extern ledc_channel_config_t mm_ledc_channel;

esp_err_t mm_generic_display_driver_st7789_init(void);
esp_err_t mm_generic_display_driver_lvgl_init(void);
esp_err_t mm_generic_display_driver_rotate(lv_disp_rot_t rotation);

#ifdef __cplusplus
}
#endif

#endif /* __MM_GENERIC_DISPLAY_DRIVER_H */
