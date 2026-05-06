
#include "mm_generic_sdcard.h"
#include "mm_port_board_config.h"

#include "esp_vfs_fat.h"
#if MM_PORT_SDCARD_INTERFACE_SPI
#include "driver/sdspi_host.h"
#elif MM_PORT_SDCARD_INTERFACE_MMC
#include "driver/sdmmc_host.h"
#endif
#include "sdkconfig.h"
#include "mm_log.h"

esp_err_t mm_generic_sdcard_init(void)
{
    #if MM_PORT_SDCARD_INTERFACE_MMC

    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = VFS_FAT_MOUNT_DEFAULT_CONFIG();
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    sdmmc_card_t *card;
    const char mount_point[] = CONFIG_MM_PORT_SDCARD_MOUNT_POINT;

    mount_config.format_if_mount_failed = false;

    MM_LOG_I("Initializing SD card");
    MM_LOG_I("Using SDMMC peripheral");

    slot_config.width = 4;
    slot_config.clk = MM_PORT_SDCARD_PIN_CLK;
    slot_config.cmd = MM_PORT_SDCARD_PIN_CMD;
    slot_config.d0 = MM_PORT_SDCARD_PIN_D0;
    slot_config.d1 = MM_PORT_SDCARD_PIN_D1;
    slot_config.d2 = MM_PORT_SDCARD_PIN_D2;
    slot_config.d3 = MM_PORT_SDCARD_PIN_D3;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    MM_LOG_I("Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            MM_LOG_E("Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ////// ESP_ERR_TIMEOUT
            MM_LOG_E("Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    MM_LOG_I("Filesystem mounted");

    return ESP_OK;

    #elif MM_PORT_SDCARD_INTERFACE_SPI

    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = VFS_FAT_MOUNT_DEFAULT_CONFIG();
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    sdmmc_card_t *card;
    const char mount_point[] = CONFIG_MM_PORT_SDCARD_MOUNT_POINT;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = MM_PORT_SDCARD_PIN_MOSI,
        .miso_io_num = MM_PORT_SDCARD_PIN_MISO,
        .sclk_io_num = MM_PORT_SDCARD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    MM_LOG_I("Initializing SD card");
    MM_LOG_I("Using SPI peripheral");

    #if defined(MM_PORT_LCD_SPI_NUM) && MM_PORT_MM_PORT_SDCARD_SPI_NUM != MM_PORT_LCD_SPI_NUM

    ret = spi_bus_initialize(MM_PORT_SDCARD_SPI_NUM, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        MM_LOG_E("Failed to initialize bus.");
        return MM_FAIL;
    }

    #else

    (void) bus_cfg;

    #endif

    host.slot = MM_PORT_SDCARD_SPI_NUM;
    slot_config.gpio_cs = MM_PORT_SDCARD_PIN_CS;
    slot_config.gpio_cd = MM_PORT_SDCARD_PIN_CD;
    slot_config.gpio_wp = MM_PORT_SDCARD_PIN_WP;
    slot_config.gpio_int = MM_PORT_SDCARD_PIN_INT;
    slot_config.host_id = MM_PORT_SDCARD_SPI_NUM;
    mount_config.format_if_mount_failed = false;

    MM_LOG_I("Mounting filesystem");

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            MM_LOG_E("Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            MM_LOG_E("Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    MM_LOG_I("Filesystem mounted");

    uint64_t tot, fr;
    esp_vfs_fat_info(mount_point, &tot, &fr);
    printf("%s: %"PRIu64"MB/%"PRIu64"MB\n", mount_point, fr/1024/1024, tot/1024/1024);

    return ESP_OK;

    #else
    #error To use generic sdcard define MM_PORT_SDCARD_INTERFACE_* variable
    #endif
}
