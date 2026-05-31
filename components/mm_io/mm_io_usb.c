/**
 * @file
 *
 * @brief Handles USB as HID device
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_io_usb.h"
#include "sdkconfig.h"

#if CONFIG_MM_IO_ENABLE_USB_OTG
#include "mm_log.h"
#include "mm_err.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"
#include "tinyusb_cdc_acm.h"
#include "esp_private/usb_phy.h"
#include "vfs_tinyusb.h"
#include "esp_mac.h"
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#endif
#include "tusb.h"
#include "device/dcd.h"

static char serial_str[18];
static usb_phy_handle_t jtag_phy = NULL;
#define TTY_CDC_NUM TINYUSB_CDC_ACM_0
static tinyusb_msc_storage_handle_t msc_handle;

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

const char *string_descriptor[] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "Espressif",             // 1: Manufacturer
    "USB JTAG/serial debug unit",      // 2: Product
    serial_str,              // 3: Serials, should use chip ID
#if CFG_TUD_CDC
    "Minimonitor CDC",
#endif
#if CFG_TUD_MSC
    "Minimonitor MSC",
#endif
#if CFG_TUD_NCM
    "",
    "",
#endif
#if CFG_TUD_VENDOR
    "",
#endif
};

const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

#define __REDIRECT_STREAM(path, mode, stream) do { \
    fflush(stream); \
    freopen(path, mode, stream); \
    setvbuf(stream, NULL, _IONBF, 0); \
} while(0)

/* note: freopen only affects newly spawned tasks (?) */

esp_err_t redirect_streams_to_tinyusb(void)
{
    __REDIRECT_STREAM(VFS_TUSB_PATH_DEFAULT, "r", stdin);
    __REDIRECT_STREAM(VFS_TUSB_PATH_DEFAULT, "w", stdout);
    __REDIRECT_STREAM(VFS_TUSB_PATH_DEFAULT, "w", stderr);
    return ESP_OK;
}

esp_err_t redirect_streams_to_default(void)
{
    const char default_console[] = "/dev/console";
    __REDIRECT_STREAM(default_console, "r", stdin);
    __REDIRECT_STREAM(default_console, "w", stdout);
    __REDIRECT_STREAM(default_console, "w", stderr);
    return ESP_OK;
}

esp_err_t redirect_streams_to_null(void)
{
    const char null[] = "/dev/null";
    __REDIRECT_STREAM(null, "r", stdin);
    __REDIRECT_STREAM(null, "w", stdout);
    __REDIRECT_STREAM(null, "w", stderr);
    return ESP_OK;
}

void mm_io_usb_init(void)
{
    ESP_LOGI(TAG, "USB initialization");

    redirect_streams_to_null();

    if(jtag_phy) {
        usb_serial_jtag_driver_uninstall();
        MM_CHECK_W(usb_del_phy(jtag_phy) == ESP_OK);
        jtag_phy = NULL;
    }

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.string = string_descriptor;
    tusb_cfg.descriptor.string_count = sizeof(string_descriptor) / sizeof(string_descriptor[0]);

    /* use device MAC as serial number */
    uint8_t mac[6] = {0,0,0,0,0,0};
    esp_err_t err = esp_efuse_mac_get_default(mac);

    MM_CHECK_W(err == ESP_OK);
    snprintf(serial_str, sizeof(serial_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // extern sdmmc_card_t *card; // TODO

    // tinyusb_msc_storage_config_t config = {
    //     .medium.card = card,
    // };

    // MM_CHECK_W(tinyusb_msc_new_storage_sdmmc(&config, &msc_handle) == MM_OK);

    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TTY_CDC_NUM,
        .callback_rx = NULL,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };
    MM_CHECK_W(tinyusb_cdcacm_init(&acm_cfg) == ESP_OK);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    esp_vfs_tusb_cdc_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_tusb_cdc_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    // usb_serial_jtag_driver_config_t usb_serial_jtag_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    // usb_serial_jtag_driver_install(&usb_serial_jtag_config);
    // usb_serial_jtag_vfs_use_driver();

    esp_vfs_tusb_cdc_register(TTY_CDC_NUM, NULL);

    redirect_streams_to_tinyusb();

    MM_LOG_I("tinyusb stack initialized");
}

static void __join_tusb_task(void *arg)
{
    EventGroupHandle_t *p_eg = (EventGroupHandle_t *) arg;
    xEventGroupSetBits(*p_eg, 1);
    vTaskSuspend(NULL);
}

/** Brings tusb task into a consistent state and suspend it
 * @note This function gracefully kills the tusb task, since esp wrappers abruptly call vTaskDelete
 * without joining the thread
*/
static void __suspend_tusb_task(void)
{
    EventGroupHandle_t eg = xEventGroupCreate();

    const dcd_event_t ev = {
        .event_id = USBD_EVENT_FUNC_CALL,
        .func_call = {
            .func = __join_tusb_task,
            .param = (void *) &eg,
        }
    };

    dcd_event_handler(&ev, false);
    xEventGroupWaitBits(eg, 1, pdTRUE, pdFALSE, portMAX_DELAY);
}

void mm_io_usb_deinit(void)
{
    ESP_LOGI(TAG, "deinit start");

    redirect_streams_to_null();

    __suspend_tusb_task();

    /* delete vfs driver */
    tinyusb_driver_uninstall();

    esp_vfs_tusb_cdc_unregister(NULL);

    /* deinit cdc */
    tinyusb_cdcacm_deinit(TTY_CDC_NUM);

    tinyusb_msc_delete_storage(msc_handle);

    #if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    if(jtag_phy == NULL) {
        /* restore esp-idf usb stack */
        usb_phy_config_t phy_config = {
            .controller = USB_PHY_CTRL_SERIAL_JTAG, //USB_PHY_CTRL_OTG
            .target = USB_PHY_TARGET_INT,
        };

        usb_new_phy(&phy_config, &jtag_phy);

        usb_serial_jtag_driver_config_t usb_serial_jtag_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        usb_serial_jtag_driver_install(&usb_serial_jtag_config);
    }

    usb_serial_jtag_vfs_use_driver();
    #endif

    redirect_streams_to_default();

    MM_LOG_I("tinyusb stack de-inited");
}

#endif /* CONFIG_MM_IO_ENABLE_USB_OTG */
