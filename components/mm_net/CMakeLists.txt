idf_component_register(SRCS "mm_net_http_func.c"
                    "mm_net_http_server.c"
                    "mm_net_sta.c"
                    "mm_net.c"
                    "protocol_examples_utils.c"
                    PRIV_REQUIRES esp_wifi esp_http_server minimonitor mm_utils
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
                    EMBED_TXTFILES index.html favicon.ico panic.html
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_NET_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
