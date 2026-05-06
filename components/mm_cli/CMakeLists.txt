idf_component_register(SRCS
                    "mm_cli_cmd.c"
                    "mm_cli.c"
                    PRIV_REQUIRES console minimonitor mm_utils
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_CLI_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
