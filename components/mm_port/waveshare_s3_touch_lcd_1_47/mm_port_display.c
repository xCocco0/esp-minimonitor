
#include "mm_port_display.h"

#include <lvgl.h>
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "bsp_display.h"
#include "bsp_touch.h"
#include "bsp_i2c.h"
#include "mm_port_board_config.h"

// #if defined(CONFIG_MM_PORT_LCD_ROTATION_90) || defined(CONFIG_MM_PORT_LCD_ROTATION_270)
//     #define MM_PORT_LCD_H_RES_ADJ CONFIG_MM_PORT_LCD_V_RES
//     #define MM_PORT_LCD_V_RES_ADJ CONFIG_MM_PORT_LCD_H_RES
// #else
//     #define MM_PORT_LCD_H_RES_ADJ CONFIG_MM_PORT_LCD_H_RES
//     #define MM_PORT_LCD_V_RES_ADJ CONFIG_MM_PORT_LCD_V_RES
// #endif

// MM_PORT_LCD_H_RES_ADJ

#if CONFIG_MM_PORT_LCD_ROTATION_270
    #define DISPLAY_ROTATION_NUMERIC 270
#elif CONFIG_MM_PORT_LCD_ROTATION_180
    #define DISPLAY_ROTATION_NUMERIC 180
#elif CONFIG_MM_PORT_LCD_ROTATION_90
    #define DISPLAY_ROTATION_NUMERIC 90
#else
    #define DISPLAY_ROTATION_NUMERIC 0
#endif

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

static void __lv_tick_increment(void *arg)
{
    lv_tick_inc(CONFIG_MM_PORT_LVGL_TICK_RESOLUTION_MS);
}

static esp_err_t __mm_port_lvgl_init(void)
{
    lv_init();

    const esp_timer_create_args_t timer_args = {
        .callback = &__lv_tick_increment,
        .name = "LVGL tick",
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, CONFIG_MM_PORT_LVGL_TICK_RESOLUTION_MS * 1000));


    /* Add LCD screen */
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = MM_PORT_LCD_H_RES_ADJ * MM_PORT_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = MM_PORT_LCD_DRAW_BUFF_DOUBLE,
        .hres = MM_PORT_LCD_H_RES_ADJ,
        .vres = MM_PORT_LCD_V_RES_ADJ,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_spiram = false,
            .buff_dma = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
        }};
#if CONFIG_MM_PORT_LCD_ROTATION_270
    disp_cfg.rotation.swap_xy = true;
    disp_cfg.rotation.mirror_x = true;
    disp_cfg.rotation.mirror_y = false;
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 34));
#elif CONFIG_MM_PORT_LCD_ROTATION_180
    disp_cfg.rotation.swap_xy = false;
    disp_cfg.rotation.mirror_x = true;
    disp_cfg.rotation.mirror_y = true;
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 34, 0));
#elif CONFIG_MM_PORT_LCD_ROTATION_90
    disp_cfg.rotation.swap_xy = true;
    disp_cfg.rotation.mirror_x = false;
    disp_cfg.rotation.mirror_y = true;
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 34));
#else
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 34, 0));
#endif
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}

mm_err_t mm_port_display_init(void *arg)
{
    i2c_master_bus_handle_t i2c_bus_handle;
    (void) arg;

    i2c_bus_handle = bsp_i2c_init();
    bsp_display_init(&io_handle, &panel_handle, MM_PORT_LCD_H_RES_ADJ * MM_PORT_LCD_DRAW_BUFF_HEIGHT);
    /* note: the driver rotation is clockwise, while the config is counter-clockwise*/
    bsp_touch_init(&touch_handle, i2c_bus_handle, MM_PORT_LCD_H_RES_ADJ, MM_PORT_LCD_V_RES_ADJ, 360 - DISPLAY_ROTATION_NUMERIC);

    __mm_port_lvgl_init();

    bsp_display_brightness_init();
    bsp_display_set_brightness(128);

    return ESP_OK;
}

mm_err_t mm_port_display_set_bk(uint8_t level)
{
    bsp_display_set_brightness(level);
    return ESP_OK;
}

mm_err_t mm_port_display_rotate(int degrees)
{
    esp_err_t err;
    lv_disp_set_rotation(lvgl_disp, MM_PORT_LV_ROTATION(degrees));
    if(degrees == 0 || degrees == 180)
    {
        err = esp_lcd_panel_set_gap(panel_handle, 0, 34);
    } else {
        err = esp_lcd_panel_set_gap(panel_handle, 34, 0);
    }
    return err == ESP_OK ? MM_OK : MM_FAIL;
}
