idf_component_register(SRCS
                    "mm_io.c"
                    "mm_io_button.c"
                    "mm_io_ledstrip.c"
                    "mm_io_imu.c"
                    PRIV_REQUIRES minimonitor mm_utils mm_port
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_IO_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
