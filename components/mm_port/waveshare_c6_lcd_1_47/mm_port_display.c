
#include "mm_port_display.h"
#include "mm_generic_display_driver.h"
#include "mm_generic_backlight.h"
#include "sdkconfig.h"

mm_err_t mm_port_display_init(void *arg)
{
    (void) arg;
    mm_generic_display_driver_st7789_init();
    mm_generic_display_driver_lvgl_init();
    mm_generic_backlight_init();

    mm_port_display_rotate(0);

    return ESP_OK;
}

mm_err_t mm_port_display_set_bk(uint8_t level)
{
    mm_generic_backlight_set(level);
    return ESP_OK;
}

mm_err_t mm_port_display_rotate(int degrees)
{
    degrees = 360 - degrees;
    #if CONFIG_MM_PORT_LCD_ROTATION_90==1
    degrees = (degrees + 90) % 360;
    #elif CONFIG_MM_PORT_LCD_ROTATION_180==1
    degrees = (degrees + 180) % 360;
    #elif CONFIG_MM_PORT_LCD_ROTATION_270==1
    degrees = (degrees + 270) % 360;
    #endif
    return mm_generic_display_driver_rotate(MM_PORT_LV_ROTATION(degrees));
}
