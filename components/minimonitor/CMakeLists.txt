idf_component_register(SRCS
                    "mm_panic.c"
                    "mm_event.c"
                    "mm_sensor.c"
                    "mm_icon.c"
                    "minimonitor.c"
                    "hid/mm_hid_touchpad.c"
                    "hid/mm_hid_control.c"
                    "hid/mm_hid_scheduler.c"
                    "hid/mm_hid_macro.c"
                    PRIV_REQUIRES esp_event nvs_flash mm_utils espcoredump
                    INCLUDE_DIRS "include" "include/keys" "include/icons"
                    PRIV_INCLUDE_DIRS ""
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
