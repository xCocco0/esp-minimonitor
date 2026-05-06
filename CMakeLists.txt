# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

if(DEFINED MM_TARGET)
    set(ENV{MM_TARGET} ${MM_TARGET})
    message(STATUS "Found MM_TARGET=${MM_TARGET}.")
elseif(DEFINED ENV{MM_TARGET})
    set(MM_TARGET $ENV{MM_TARGET})
    message(STATUS "Found \$ENV\{MM_TARGET\}=$ENV{MM_TARGET}.")
# elseif(DEFINED MM_TARGET_DEFAULT)
#     message(NOTICE "Target not specified. Using default ${MM_TARGET_DEFAULT}.")
#     set(MM_TARGET ${MM_TARGET_DEFAULT})
#     set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.${MM_TARGET}")
else()
    message(FATAL_ERROR " Please choose a target board.\n"
                        " idf.py -DMM_TARGET=\"boardname\" ...\n"
                        " or set MM_TARGET environment variable.")
endif()

# add board-specific sdkconfig defaults
# note: the order for config inclusion is
# (1) sdkconfig.defaults
# (2) sdkconfig.defaults.[chip] (e.g. esp32c6)
# (3) sdkconfig.[board] (e.g. sdkconfig.generic_board)
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.${MM_TARGET}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(esp-minimonitor)
