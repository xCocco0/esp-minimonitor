
set(board_src "common/mm_generic_display_driver.c"
        "common/mm_generic_backlight.c"
        "common/mm_generic_sdcard.c"
        "waveshare_c6_lcd_1_47/mm_port_display.c"
        "waveshare_c6_lcd_1_47/mm_port_fs.c"
)
set(board_requires esp_lcd espressif__esp_lvgl_port fatfs)
set(board_priv_includes "common" "waveshare_c6_lcd_1_47")
set(board_includes "waveshare_c6_lcd_1_47/include")

# cmake_minimum_required(VERSION 3.12)
# list(TRANSFORM board_src PREPEND waveshare_c6_lcd_1_47/)
