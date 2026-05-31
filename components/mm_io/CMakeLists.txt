
set(TINYUSB_TARGETS esp32s2 esp32s3 esp32p4 esp32h4 esp32s31)
if(${IDF_TARGET} IN_LIST TINYUSB_TARGETS)
    set(TINYUSB_REQ "usb tinyusb esp_tinyusb esp_driver_usb_serial_jtag")
else()
    set(TINYUSB_REQ "")
endif()

idf_component_register(SRCS
                    "mm_io.c"
                    "mm_io_button.c"
                    "mm_io_ledstrip.c"
                    "mm_io_imu.c"
                    "mm_io_usb.c"
                    "vfs_tinyusb.c"
                    PRIV_REQUIRES minimonitor mm_utils mm_port ${TINYUSB_REQ}
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLOG_LOCAL_LEVEL=${CONFIG_MM_IO_LOG_LEVEL}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TAG="${COMPONENT_NAME}")
