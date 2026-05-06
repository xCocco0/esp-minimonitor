
if(NOT "$ENV{MM_TARGET}" STREQUAL "")
    message(STATUS "Adding sources from ${CMAKE_CURRENT_LIST_DIR}/$ENV{MM_TARGET}/")
    include(${CMAKE_CURRENT_LIST_DIR}/$ENV{MM_TARGET}/include.cmake)
else()
    message(FATAL_ERROR " Please choose a target board.\n"
                        " idf.py -DMM_TARGET=\"boardname\" ...\n"
                        " or set MM_TARGET environment variable.")
endif()

idf_component_register(SRCS "mm_port_defaults.c" ${board_src}
                    PRIV_REQUIRES mm_utils lvgl ${board_requires}
                    INCLUDE_DIRS "include" ${board_includes}
                    PRIV_INCLUDE_DIRS "priv_include" ${board_priv_includes}
                    WHOLE_ARCHIVE
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_PORT_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
