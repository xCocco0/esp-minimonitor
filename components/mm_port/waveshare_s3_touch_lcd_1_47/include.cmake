
set(board_src "waveshare_s3_touch_lcd_1_47/mm_port_display.c"
        "waveshare_s3_touch_lcd_1_47/mm_port_fs.c"
        "waveshare_s3_touch_lcd_1_47/bsp_display.c"
        "waveshare_s3_touch_lcd_1_47/bsp_i2c.c"
        "waveshare_s3_touch_lcd_1_47/bsp_qmi8658.c"
        "waveshare_s3_touch_lcd_1_47/bsp_touch.c"
        "waveshare_s3_touch_lcd_1_47/esp_lcd_jd9853.c"
        "waveshare_s3_touch_lcd_1_47/esp_lcd_touch_axs5106.c"
        "common/mm_generic_sdcard.c"
)

set(board_requires esp_lcd espressif__esp_lvgl_port espressif__esp_lcd_touch esp_driver_sdmmc fatfs)
set(board_priv_includes "waveshare_s3_touch_lcd_1_47" "waveshare_s3_touch_lcd_1_47/priv_include" "common")
set(board_includes "waveshare_s3_touch_lcd_1_47/include")

# cmake_minimum_required(VERSION 3.12)
# list(TRANSFORM board_src PREPEND waveshare_c6_lcd_1_47/)
