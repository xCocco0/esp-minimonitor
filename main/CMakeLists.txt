idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES mm_cli minimonitor mm_ui mm_port mm_net mm_io mm_utils esp_app_format
                    INCLUDE_DIRS ".")
# TODO prune PRIV_REQUIRES

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=ESP_LOG_MAX")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")