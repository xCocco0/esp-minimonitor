
#include "mm_port_display.h"

#include <lvgl.h>
#include "driver/i2c.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "mm_generic_backlight.h"
#include "mm_port_board_config.h"

#define EXAMPLE_I2C_NUM 0 // I2C number

static esp_lcd_touch_handle_t tp;
static lv_indev_drv_t indev_drv; // Input device driver (Touch)
static lv_disp_drv_t disp_drv;   /*Descriptor of a display driver*/
static lv_disp_t *disp;
static esp_lcd_panel_handle_t panel_handle;

static void __lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    // uint8_t touchpad_cnt = 0;
    // esp_lcd_touch_point_data_t touch_data;
    // esp_lcd_touch_read_data(tp);
    // /* Get coordinates */
    // //bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);
    // bool touchpad_pressed = esp_lcd_touch_get_data(tp, &touch_data, &touchpad_cnt, 1);

    // if (touchpad_pressed && touchpad_cnt > 0)
    // {
    //     data->point.x = touch_data.x;
    //     data->point.y = touch_data.y;
    //     data->state = LV_INDEV_STATE_PRESSED;
    // }
    // else
    // {
    //     data->state = LV_INDEV_STATE_RELEASED;
    // }
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;
    esp_lcd_touch_read_data(tp);
    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0)
    {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static bool __notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_flush_ready(&disp_drv);
    return false;
}

static void __display_init(void)
{
    ESP_LOGI(TAG, "SPI BUS init");
    spi_bus_config_t buscfg = {
        .sclk_io_num = MM_PORT_LCD_GPIO_SCLK,
        .mosi_io_num = MM_PORT_LCD_GPIO_MOSI,
        .miso_io_num = MM_PORT_LCD_GPIO_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(MM_PORT_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "Install panel IO");

    esp_lcd_panel_io_handle_t io_handle = NULL;

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = MM_PORT_LCD_GPIO_DC,
        .cs_gpio_num = MM_PORT_LCD_GPIO_CS,
        .pclk_hz = MM_PORT_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = MM_PORT_LCD_CMD_BITS,
        .lcd_param_bits = MM_PORT_LCD_PARAM_BITS,
        .spi_mode = MM_PORT_LCD_SPI_MODE,
        .trans_queue_depth = MM_PORT_LCD_SPI_TRANS_QUEUE,
        .on_color_trans_done = __notify_lvgl_flush_ready,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) MM_PORT_LCD_SPI_NUM, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = MM_PORT_LCD_GPIO_RST,
        #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = MM_PORT_LCD_RGB_ENDIANNESS,
        #else
        .rgb_ele_order = MM_PORT_LCD_RGB_ENDIANNESS,
        #endif
        .data_endian = MM_PORT_LCD_DATA_ENDIANNESS,
        .bits_per_pixel = MM_PORT_LCD_BITS_PER_PIXEL,
        .flags = {
            .reset_active_high = 0,
        },
    };
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, MM_PORT_LCD_MIRROR_X_ADJ, MM_PORT_LCD_MIRROR_Y_ADJ));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, MM_PORT_LCD_SWAP_XY_ADJ));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, MM_PORT_LCD_INVERT_COLOR));
}

static void __indev_init(void)
{
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = __lvgl_touch_cb;
    indev_drv.user_data = tp;

    lv_indev_drv_register(&indev_drv);
}

static void __lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display

    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

void __lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t draw_buf;
    lv_color_t *buf1 = heap_caps_malloc(MM_PORT_LCD_H_RES * MM_PORT_LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(MM_PORT_LCD_H_RES * MM_PORT_LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2,
                          MM_PORT_LCD_H_RES * MM_PORT_LCD_V_RES); /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    lv_disp_drv_init(&disp_drv); /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MM_PORT_LCD_H_RES_ADJ;
    disp_drv.ver_res = MM_PORT_LCD_V_RES_ADJ;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = __lvgl_flush_cb;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf;

    /*Required for Example 3)*/
    disp_drv.full_refresh = 1;
    // disp_drv.direct_mode = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    // disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    disp = lv_disp_drv_register(&disp_drv);
}

static void __touch_init(void)
{
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    ESP_LOGI(TAG, "Initialize I2C");
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = MM_PORT_LCD_TP_SDA,
        .scl_io_num = MM_PORT_LCD_TP_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    /* Initialize I2C */
    ESP_ERROR_CHECK(i2c_param_config(EXAMPLE_I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(EXAMPLE_I2C_NUM, i2c_conf.mode, 0, 0, 0));

    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    tp_io_config.scl_speed_hz = 0;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)EXAMPLE_I2C_NUM, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = MM_PORT_LCD_H_RES,
        .y_max = MM_PORT_LCD_V_RES,
        .rst_gpio_num = MM_PORT_LCD_GPIO_RST,
        .int_gpio_num = MM_PORT_LCD_TP_INT,
        .flags = {
            .swap_xy = MM_PORT_LCD_SWAP_XY_ADJ,
            .mirror_x = MM_PORT_LCD_MIRROR_X_ADJ,
            .mirror_y = MM_PORT_LCD_MIRROR_Y_ADJ,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller CST816");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp));
}

static void __lv_tick_increment(void *arg)
{
    lv_tick_inc(CONFIG_MM_PORT_LVGL_TICK_RESOLUTION_MS);
}

mm_err_t mm_port_display_init(void *arg)
{
    (void) arg;

    lv_init();
    __display_init();
    __touch_init();
    __lv_port_disp_init();
    __indev_init();

    const esp_timer_create_args_t timer_args = {
        .callback = &__lv_tick_increment,
        .name = "LVGL tick",
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, CONFIG_MM_PORT_LVGL_TICK_RESOLUTION_MS * 1000));

    ESP_ERROR_CHECK(mm_generic_backlight_init());
    ESP_ERROR_CHECK(mm_generic_backlight_set(255));

    //lv_disp_set_default(disp);

    return ESP_OK;
}

mm_err_t mm_port_display_set_bk(uint8_t level)
{
    mm_generic_backlight_set(level);
    return ESP_OK;
}

mm_err_t mm_port_display_rotate(int degrees)
{
    lv_disp_set_rotation(disp, MM_PORT_LV_ROTATION(360 - degrees));
    // if(degrees == 0 || degrees == 180)
    // {
    //     err = esp_lcd_panel_set_gap(panel_handle, 0, 34);
    // } else {
    //     err = esp_lcd_panel_set_gap(panel_handle, 34, 0);
    // }
    // return err == ESP_OK ? MM_OK : MM_FAIL;
    esp_lcd_panel_swap_xy(panel_handle, MM_PORT_LCD_SWAP_XY_ADJ  ^ (degrees == 90  || degrees == 270));
    esp_lcd_panel_mirror(panel_handle,  MM_PORT_LCD_MIRROR_X_ADJ ^ (degrees == 180 || degrees == 270),
                                        MM_PORT_LCD_MIRROR_Y_ADJ ^ (degrees == 90  || degrees == 180)
    );

    return MM_OK;
}
