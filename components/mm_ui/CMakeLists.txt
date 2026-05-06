
file(GLOB SRC_IMAGES "icons/*.c")
#set(SRC_IMAGES "icons/mm_icon_a.c" "icons/mm_icon_activity.c" "icons/mm_icon_alarm.c" "icons/mm_icon_aperture.c" "icons/mm_icon_arrow.c" "icons/mm_icon_at.c" "icons/mm_icon_badge.c" "icons/mm_icon_ban.c" "icons/mm_icon_battery.c" "icons/mm_icon_battery_slash.c" "icons/mm_icon_bed.c" "icons/mm_icon_bell.c" "icons/mm_icon_bell_slash.c" "icons/mm_icon_bluetooth.c" "icons/mm_icon_bullhorn.c" "icons/mm_icon_cake.c" "icons/mm_icon_checkmark.c" "icons/mm_icon_clock.c" "icons/mm_icon_cloud.c" "icons/mm_icon_cog.c" "icons/mm_icon_empty_sun.c" "icons/mm_icon_envelope.c" "icons/mm_icon_moon.c" "icons/mm_icon_nut.c" "icons/mm_icon_phone.c" "icons/mm_icon_power.c" "icons/mm_icon_stopwatch.c" "icons/mm_icon_sun.c" "icons/mm_icon_trash.c" "icons/mm_icon_whatsapp.c" "icons/mm_icon_wifi.c" "icons/mm_icon_wifi_slash.c"  "icons/mm_icon_double_arrow_slash.c"  "icons/mm_icon_double_arrow.c" )

set(SRC_IMAGES
        "icons/v8/mm_icon_activity.c"
        "icons/v8/mm_icon_alarm.c"
        "icons/v8/mm_icon_aperture.c"
        "icons/v8/mm_icon_arrow.c"
        "icons/v8/mm_icon_at.c"
        "icons/v8/mm_icon_badge.c"
        "icons/v8/mm_icon_ban.c"
        "icons/v8/mm_icon_battery.c"
        "icons/v8/mm_icon_battery_slash.c"
        "icons/v8/mm_icon_bed.c"
        "icons/v8/mm_icon_bell.c"
        "icons/v8/mm_icon_bell_slash.c"
        "icons/v8/mm_icon_bluetooth.c"
        "icons/v8/mm_icon_bullhorn.c"
        "icons/v8/mm_icon_cake.c"
        "icons/v8/mm_icon_checkmark.c"
        "icons/v8/mm_icon_clock.c"
        "icons/v8/mm_icon_cloud.c"
        "icons/v8/mm_icon_cog.c"
        "icons/v8/mm_icon_double_arrow.c"
        "icons/v8/mm_icon_double_arrow_slash.c"
        "icons/v8/mm_icon_empty_sun.c"
        "icons/v8/mm_icon_envelope.c"
        "icons/v8/mm_icon_lock.c"
        "icons/v8/mm_icon_moon.c"
        "icons/v8/mm_icon_nut.c"
        "icons/v8/mm_icon_phone.c"
        "icons/v8/mm_icon_power.c"
        "icons/v8/mm_icon_stopwatch.c"
        "icons/v8/mm_icon_sun.c"
        "icons/v8/mm_icon_trash.c"
        "icons/v8/mm_icon_whatsapp.c"
        "icons/v8/mm_icon_wifi.c"
        "icons/v8/mm_icon_wifi_slash.c"
)

#RELATIVE <path>

idf_component_register(SRCS
                        "mm_ui.c"
                        "mm_ui_screen_main.c"
                        "mm_ui_screen_panic.c"
                        "mm_ui_screen.c"
                        "mm_ui_state.c"
                        "mm_ui_layout.c"
                        "mm_ui_notification.c"
                        "mm_icon.c"
                        "mm_style.c"
                        "mm_widgets.c"
                        "mm_theme.c"
                        "widgets/mm_widget_meter.c"
                        "widgets/mm_widget_notiftray.c"
                        "widgets/mm_widget_notifboard.c"
                        "widgets/mm_widget_meterlist.c"
                        "widgets/mm_widget_glowframe.c"
                        "widgets/mm_widget_toast.c"
                        "widgets/mm_widget_clock.c"
                        "widgets/mm_widget_button.c"
                        "fonts/mm_font_montserrat_72.c"
                        "fonts/mm_font_montserrat_80.c"
                        "fonts/mm_font_montserrat_90.c"
                        "fonts/mm_font_montserrat_100.c"
                        "fonts/mm_font_montserrat_120.c"
                        "fonts/mm_font_montserrat_150.c"
                        ${SRC_IMAGES}
                        PRIV_REQUIRES minimonitor mm_port mm_utils
                        INCLUDE_DIRS "include"
                        PRIV_INCLUDE_DIRS "priv_include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_UI_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
