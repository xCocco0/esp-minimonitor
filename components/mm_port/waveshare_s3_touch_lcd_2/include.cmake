
set(board_src "common/mm_generic_backlight.c"
        "common/mm_generic_sdcard.c"
        "waveshare_s3_touch_lcd_2/mm_port_display.c"
        "waveshare_s3_touch_lcd_2/mm_port_imu.c"
        "waveshare_s3_touch_lcd_2/bsp_qmi8658.c"
)
set(board_requires esp_lcd espressif__esp_lcd_touch_cst816s fatfs)
set(board_priv_includes "common" "waveshare_s3_touch_lcd_2" "waveshare_s3_touch_lcd_2/priv_include")
set(board_includes "waveshare_s3_touch_lcd_2/include")

# cmake_minimum_required(VERSION 3.12)
# list(TRANSFORM board_src PREPEND waveshare_c6_lcd_1_47/)
