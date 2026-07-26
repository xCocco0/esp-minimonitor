idf_component_register(SRCS
                    "mm_panic.c"
                    "mm_event.c"
                    "mm_sensor.c"
                    "mm_hid_control.c"
                    "mm_icon.c"
                    "mm_touchpad.c"
                    "minimonitor.c"
                    PRIV_REQUIRES esp_event nvs_flash mm_utils espcoredump
                    INCLUDE_DIRS "include" "include/keys" "include/icons"
                    PRIV_INCLUDE_DIRS ""
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
