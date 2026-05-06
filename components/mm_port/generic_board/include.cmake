
set(board_src "common/mm_generic_display_driver.c" "common/mm_generic_backlight.c" "generic_board/mm_port_display.c")
set(board_requires esp_lcd espressif__esp_lvgl_port)
set(board_priv_includes "common" "generic_board")
set(board_includes "generic_board/include")
