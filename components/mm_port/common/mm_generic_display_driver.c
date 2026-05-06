
#include "mm_generic_display_driver.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_io.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include <lvgl.h>
#include "sdkconfig.h"
#include "mm_port_board_config.h"

esp_lcd_panel_handle_t mm_lcd_panel = NULL;
esp_lcd_panel_io_handle_t mm_lcd_io = NULL;
lv_display_t *mm_lvgl_disp = NULL;

esp_err_t mm_generic_display_driver_st7789_init(void)
{
    esp_err_t ret = ESP_OK;

    /* LCD initialization */

    #if MM_PORT_LCD_INTERFACE_SPI
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = MM_PORT_LCD_GPIO_SCLK,
        .mosi_io_num = MM_PORT_LCD_GPIO_MOSI,
        .miso_io_num = MM_PORT_LCD_GPIO_MISO,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        //.max_transfer_sz = MM_PORT_LCD_H_RES * MM_PORT_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
        .max_transfer_sz = 0,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(MM_PORT_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = MM_PORT_LCD_GPIO_DC,
        .cs_gpio_num = MM_PORT_LCD_GPIO_CS,
        .pclk_hz = MM_PORT_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = MM_PORT_LCD_CMD_BITS,
        .lcd_param_bits = MM_PORT_LCD_PARAM_BITS,
        .spi_mode = MM_PORT_LCD_SPI_MODE,
        .trans_queue_depth = MM_PORT_LCD_SPI_TRANS_QUEUE,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)MM_PORT_LCD_SPI_NUM, &io_config, &mm_lcd_io), err, TAG, "New panel IO failed");

    #elif MM_PORT_LCD_INTERFACE_I2C
    #warning This piece of code is untested, I just copy pasted from IDF examples

    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = -1,
        .sda_io_num = MM_PORT_LCD_GPIO_SDA,
        .scl_io_num = MM_PORT_LCD_GPIO_SCL,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = MM_PORT_LCD_I2C_HW_ADDR,
        .scl_speed_hz = MM_PORT_LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = MM_PORT_LCD_CMD_BITS,
        .lcd_param_bits = MM_PORT_LCD_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &mm_lcd_io, &mm_lcd_io));

    #else
    #error Choose must choose either SPI or I2C in the configuration.
    #endif

    ESP_LOGI(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
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
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(mm_lcd_io, &panel_config, &mm_lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(mm_lcd_panel);
    esp_lcd_panel_init(mm_lcd_panel);
    esp_lcd_panel_disp_on_off(mm_lcd_panel, true);
    esp_lcd_panel_set_gap(mm_lcd_panel, MM_PORT_LCD_OFFSET_X, MM_PORT_LCD_OFFSET_Y);
    esp_lcd_panel_invert_color(mm_lcd_panel, MM_PORT_LCD_INVERT_COLOR);

    return ret;

err:
    if(mm_lcd_panel) {
        esp_lcd_panel_del(mm_lcd_panel);
    }
    if(mm_lcd_io) {
        esp_lcd_panel_io_del(mm_lcd_io);
    }
    #if MM_PORT_LCD_INTERFACE_SPI
    spi_bus_free(MM_PORT_LCD_SPI_NUM);
    #endif
    return ret;
}

static void __lv_tick_increment(void *arg)
{
    lv_tick_inc(CONFIG_MM_PORT_LVGL_TICK_RESOLUTION_MS);
}

esp_err_t mm_generic_display_driver_lvgl_init(void)
{
    /* Initialize LVGL */
    lv_init();

    const esp_timer_create_args_t timer_args = {
        .callback = &__lv_tick_increment,
        .name = "LVGL tick",
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, CONFIG_MM_PORT_LVGL_TICK_RESOLUTION_MS * 1000));

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = mm_lcd_io,
        .panel_handle = mm_lcd_panel,
        .buffer_size = MM_PORT_LCD_H_RES * MM_PORT_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = MM_PORT_LCD_DRAW_BUFF_DOUBLE,
        .hres = MM_PORT_LCD_H_RES,
        .vres = MM_PORT_LCD_V_RES,
        .monochrome = false,
        #if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
        #endif
        .rotation = {
            .swap_xy = MM_PORT_LCD_SWAP_XY,
            .mirror_x = MM_PORT_LCD_MIRROR_X,
            .mirror_y = MM_PORT_LCD_MIRROR_Y,
        },
        .flags = {
            #if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
            #endif
            .buff_dma = MM_PORT_LCD_BUFF_DMA,
            .buff_spiram = MM_PORT_LCD_BUFF_IN_SPIRAM,
            .full_refresh = MM_PORT_LCD_USE_FULL_REFRESH,
        }
    };

    mm_lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    // lv_disp_drv_init


    return ESP_OK;
}

esp_err_t mm_generic_display_driver_rotate(lv_disp_rot_t rotation)
{
    esp_err_t err;
    lv_disp_set_rotation(mm_lvgl_disp, rotation);
    #if LVGL_VERSION_MAJOR >= 9
    if(rotation == LV_DISP_ROTATION_0 || rotation == LV_DISP_ROTATION_180) // TODO: Check if it really works
    #else
    if(rotation == LV_DISP_ROT_NONE || rotation == LV_DISP_ROT_180)
    #endif
    {
        err = esp_lcd_panel_set_gap(mm_lcd_panel, MM_PORT_LCD_OFFSET_X, MM_PORT_LCD_OFFSET_Y);
    } else {
        err = esp_lcd_panel_set_gap(mm_lcd_panel, MM_PORT_LCD_OFFSET_Y, MM_PORT_LCD_OFFSET_X);
    }
    return err;
}
