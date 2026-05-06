idf_component_register(SRCS
                    "mm_persist.c"
                    "mm_err.c"
                    "mm_log.c"
                    "mm_conf.c"
                    PRIV_REQUIRES nvs_flash esp_timer
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_UTILS_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
