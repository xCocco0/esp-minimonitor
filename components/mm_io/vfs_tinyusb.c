/*
 * SPDX-FileCopyrightText: 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Modifications with respect to the original:
 * - add blocking reads (breaks user CDC_EVENT_RX callbacks)
 * - support for select
 */

#include "vfs_tinyusb.h"
#include "sdkconfig.h"

#if CONFIG_MM_IO_ENABLE_USB_OTG

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"
#include "esp_idf_version.h"
#include "freertos/FreeRTOS.h"

#undef TAG
const static char *TAG = "tusb_vfs";

// Token signifying that no character is available
#define NONE -1

#define FD_CHECK(fd, ret_val) do {                      \
                                    if ((fd) != 0) {    \
                                    errno = EBADF;      \
                                    return (ret_val);   \
                                    }                   \
                                } while (0)



#if CONFIG_NEWLIB_STDOUT_LINE_ENDING_CRLF
#   define DEFAULT_TX_MODE ESP_LINE_ENDINGS_CRLF
#elif CONFIG_NEWLIB_STDOUT_LINE_ENDING_CR
#   define DEFAULT_TX_MODE ESP_LINE_ENDINGS_CR
#else
#   define DEFAULT_TX_MODE ESP_LINE_ENDINGS_LF
#endif

#if CONFIG_NEWLIB_STDIN_LINE_ENDING_CRLF
#   define DEFAULT_RX_MODE ESP_LINE_ENDINGS_CRLF
#elif CONFIG_NEWLIB_STDIN_LINE_ENDING_CR
#   define DEFAULT_RX_MODE ESP_LINE_ENDINGS_CR
#else
#   define DEFAULT_RX_MODE ESP_LINE_ENDINGS_LF
#endif

typedef struct {
    _lock_t write_lock;
    _lock_t read_lock;
    SemaphoreHandle_t buf_empty_sema;
#ifdef CONFIG_VFS_SUPPORT_SELECT
    esp_vfs_select_sem_t select_sema;
#endif
    esp_line_endings_t tx_mode; // Newline conversion mode when transmitting
    esp_line_endings_t rx_mode; // Newline conversion mode when receiving
    uint32_t flags;
    char vfs_path[VFS_TUSB_MAX_PATH];
    int cdc_intf;
} vfs_tinyusb_t;

static vfs_tinyusb_t s_vfstusb;

static void unblock_readers(int itf, cdcacm_event_t *event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(s_vfstusb.buf_empty_sema, &xHigherPriorityTaskWoken);

#ifdef CONFIG_VFS_SUPPORT_SELECT
    if (s_vfstusb.select_sema.sem) {
        esp_vfs_select_triggered_isr(s_vfstusb.select_sema, &xHigherPriorityTaskWoken);
    }
#endif

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

#ifdef CONFIG_VFS_SUPPORT_SELECT

static esp_err_t tusb_start_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, esp_vfs_select_sem_t sem, void **end_select_args)
{
    const int fd = 0;
    bool ready = false;

    if (readfds && FD_ISSET(fd, readfds)) {
        if (tud_cdc_n_available(s_vfstusb.cdc_intf) > 0) {
            ready = true;
        } else {
            _lock_acquire(&(s_vfstusb.read_lock));
            s_vfstusb.select_sema = sem;
            _lock_release(&(s_vfstusb.read_lock));
            FD_CLR(fd, readfds);
        }
    }

    /* we don't update the readfd in the isr callback but in the end_select */
    *end_select_args = (void *) readfds;

    if (writefds && FD_ISSET(fd, writefds)) {
        if (tud_cdc_n_write_available(s_vfstusb.cdc_intf) > 0) {
            ready = true;
        } else {
            FD_CLR(fd, writefds);
        }
    }

    if (ready) {
        esp_vfs_select_triggered(sem);
    }

    return ESP_OK;
}

static esp_err_t tusb_end_select(void *end_select_args)
{
    fd_set *readfds = (fd_set *) end_select_args;
    if (readfds && tud_cdc_n_available(s_vfstusb.cdc_intf) > 0) {
        FD_SET(0, readfds);
    }
    _lock_acquire(&(s_vfstusb.read_lock));
    s_vfstusb.select_sema.sem = NULL;
    _lock_release(&(s_vfstusb.read_lock));
    return ESP_OK;
}

#endif

static esp_err_t apply_path(char const *path)
{
    if (path == NULL) {
        path = VFS_TUSB_PATH_DEFAULT;
    }

    size_t path_len = strlen(path) + 1;
    if (path_len > VFS_TUSB_MAX_PATH) {
        ESP_LOGE(TAG, "The path is too long; maximum is %d characters", VFS_TUSB_MAX_PATH);
        return ESP_ERR_INVALID_ARG;
    }
    strncpy(s_vfstusb.vfs_path, path, (VFS_TUSB_MAX_PATH - 1));
    ESP_LOGV(TAG, "Path is set to `%s`", path);
    return ESP_OK;
}

/**
 * @brief Fill s_vfstusb
 *
 * @param cdc_intf - interface of tusb for registration
 * @param path - a path where the CDC will be registered
 * @return esp_err_t ESP_OK or ESP_ERR_INVALID_ARG
 */
static esp_err_t vfstusb_init(int cdc_intf, char const *path)
{
    esp_err_t ret;

    s_vfstusb.cdc_intf = cdc_intf;
    s_vfstusb.tx_mode = DEFAULT_TX_MODE;
    s_vfstusb.rx_mode = DEFAULT_RX_MODE;

    _lock_init(&(s_vfstusb.write_lock));
    _lock_init(&(s_vfstusb.read_lock));

    s_vfstusb.buf_empty_sema = xSemaphoreCreateBinary();
    if (tud_cdc_n_available(s_vfstusb.cdc_intf) == 0) {
        xSemaphoreTake(s_vfstusb.buf_empty_sema, 0);
    }

    ret = tinyusb_cdcacm_register_callback(cdc_intf, CDC_EVENT_RX, unblock_readers);
    if(ret != ESP_OK) {
        return ret;
    }

    return apply_path(path);
}

/**
 * @brief Clear s_vfstusb to default values
 */
static void vfstusb_deinit(void)
{
    tinyusb_cdcacm_register_callback(s_vfstusb.cdc_intf, CDC_EVENT_RX, NULL);
    _lock_close(&(s_vfstusb.write_lock));
    _lock_close(&(s_vfstusb.read_lock));
    xSemaphoreGive(s_vfstusb.buf_empty_sema);
    vSemaphoreDelete(s_vfstusb.buf_empty_sema);
    memset(&s_vfstusb, 0, sizeof(s_vfstusb));
}

static int tusb_open(const char *path, int flags, int mode)
{
    (void) mode;
    (void) path;
    return 0;
}

static ssize_t tusb_write(int fd, const void *data, size_t size)
{
    FD_CHECK(fd, -1);
    size_t written_sz = 0;
    const char *data_c = (const char *)data;
    _lock_acquire(&(s_vfstusb.write_lock));
    for (size_t i = 0; i < size; i++) {
        int c = data_c[i];
        if (c != '\n') {
            if (!tinyusb_cdcacm_write_queue_char(s_vfstusb.cdc_intf, c)) {
                break; // can't write anymore
            }
        } else {
            if (s_vfstusb.tx_mode == ESP_LINE_ENDINGS_CRLF || s_vfstusb.tx_mode == ESP_LINE_ENDINGS_CR) {
                char cr = '\r';
                if (!tinyusb_cdcacm_write_queue_char(s_vfstusb.cdc_intf, cr)) {
                    break; // can't write anymore
                }
            }
            if (s_vfstusb.tx_mode == ESP_LINE_ENDINGS_CRLF || s_vfstusb.tx_mode == ESP_LINE_ENDINGS_LF) {
                char lf = '\n';
                if (!tinyusb_cdcacm_write_queue_char(s_vfstusb.cdc_intf, lf)) {
                    break; // can't write anymore
                }
            }
        }
        written_sz++;
    }
    tud_cdc_n_write_flush(s_vfstusb.cdc_intf);
    _lock_release(&(s_vfstusb.write_lock));
    return written_sz;
}

static int tusb_close(int fd)
{
    FD_CHECK(fd, -1);
    return 0;
}

static ssize_t tusb_read(int fd, void *data, size_t size)
{
    FD_CHECK(fd, -1);
    char *data_c = (char *) data;
    size_t received = 0;
    if (!(s_vfstusb.flags & O_NONBLOCK)) {
        while (tud_cdc_n_available(s_vfstusb.cdc_intf) == 0) {
            xSemaphoreTake(s_vfstusb.buf_empty_sema, portMAX_DELAY);
        }
    }
    _lock_acquire(&(s_vfstusb.read_lock));

    if (tud_cdc_n_available(s_vfstusb.cdc_intf) == 0) {
        goto finish;
    }
    while (received < size) {
        int c = tud_cdc_n_read_char(s_vfstusb.cdc_intf);
        if ( c == NONE) { // if data ends
            break;
        }

        // Handle line endings. From configured mode -> LF mode
        if (s_vfstusb.rx_mode == ESP_LINE_ENDINGS_CR) {
            // Change CRs to newlines
            if (c == '\r') {
                c = '\n';
            }
        } else if (s_vfstusb.rx_mode == ESP_LINE_ENDINGS_CRLF) {
            if (c == '\r') {
                uint8_t next_char = NONE;
                // Check if next char is newline. If yes, we got CRLF sequence
                tud_cdc_n_peek(s_vfstusb.cdc_intf, &next_char);
                if (next_char == '\n') {
                    c = tud_cdc_n_read_char(s_vfstusb.cdc_intf); // Remove '\n' from the fifo
                }
            }
        }

        data_c[received] = (char) c;
        ++received;
        if (c == '\n') {
            break;
        }
    }
finish:
    _lock_release(&(s_vfstusb.read_lock));
    if (!(s_vfstusb.flags & O_NONBLOCK) && tud_cdc_n_available(s_vfstusb.cdc_intf) > 0) {
        /* if more to read unlock */
        xSemaphoreGive(s_vfstusb.buf_empty_sema);
    }
    if (received > 0) {
        return received;
    }
    errno = EWOULDBLOCK;
    return -1;
}

static int tusb_fstat(int fd, struct stat *st)
{
    FD_CHECK(fd, -1);
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    return 0;
}

static int tusb_fcntl(int fd, int cmd, int arg)
{
    FD_CHECK(fd, -1);
    int result = 0;
    switch (cmd) {
    case F_GETFL:
        result = s_vfstusb.flags;
        break;
    case F_SETFL:
        s_vfstusb.flags = arg;
        if (arg & O_NONBLOCK) {
            /* in case a reader is currently waiting */
            xSemaphoreGive(s_vfstusb.buf_empty_sema);
        }
        break;
    default:
        result = -1;
        errno = ENOSYS;
        break;
    }
    return result;
}

esp_err_t esp_vfs_tusb_cdc_unregister(char const *path)
{
    ESP_LOGD(TAG, "Unregistering CDC-VFS driver");
    int res;

    if (path == NULL) { // NULL means using the default path for unregistering: VFS_TUSB_PATH_DEFAULT
        path = VFS_TUSB_PATH_DEFAULT;
    }
    res = strcmp(s_vfstusb.vfs_path, path);

    if (res) {
        res = ESP_ERR_INVALID_ARG;
        ESP_LOGE(TAG, "There is no CDC-VFS driver registered to path '%s' (err: 0x%x)", path, res);
        return res;
    }

    res = esp_vfs_unregister(s_vfstusb.vfs_path);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Can't unregister CDC-VFS driver from '%s' (err: 0x%x)", s_vfstusb.vfs_path, res);
    } else {
        ESP_LOGD(TAG, "Unregistered CDC-VFS driver");
        vfstusb_deinit();
    }
    return res;
}

esp_err_t esp_vfs_tusb_cdc_register(int cdc_intf, char const *path)
{
    ESP_LOGD(TAG, "Registering CDC-VFS driver");
    int res;
    if (!tinyusb_cdcacm_initialized(cdc_intf)) {
        ESP_LOGE(TAG, "TinyUSB CDC#%d is not initialized", cdc_intf);
        return ESP_ERR_INVALID_STATE;
    }

    res = vfstusb_init(cdc_intf, path);
    if (res != ESP_OK) {
        return res;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)

#ifdef CONFIG_VFS_SUPPORT_SELECT
    static const esp_vfs_select_ops_t fs_select_ops = {
        .start_select = &tusb_start_select,
        .end_select = &tusb_end_select,
    };
#endif

    static const esp_vfs_fs_ops_t fs_ops = {
        .close = &tusb_close,
        .fcntl = &tusb_fcntl,
        .fstat = &tusb_fstat,
        .open = &tusb_open,
        .read = &tusb_read,
        .write = &tusb_write,
    #ifdef CONFIG_VFS_SUPPORT_SELECT
        .select = &fs_select_ops
    #endif
    };

    res = esp_vfs_register_fs(s_vfstusb.vfs_path, &fs_ops, ESP_VFS_FLAG_STATIC, NULL);

#else
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .close = &tusb_close,
        .fcntl = &tusb_fcntl,
        .fstat = &tusb_fstat,
        .open = &tusb_open,
        .read = &tusb_read,
        .write = &tusb_write,
    #ifdef CONFIG_VFS_SUPPORT_SELECT
        .start_select = &tusb_start_select,
        .end_select = &tusb_end_select,
    #endif
    };

    res = esp_vfs_register(s_vfstusb.vfs_path, &vfs, NULL);

#endif

    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Can't register CDC-VFS driver (err: %x)", res);
    } else {
        ESP_LOGD(TAG, "CDC-VFS registered (%s)", s_vfstusb.vfs_path);
    }
    return res;
}

void esp_vfs_tusb_cdc_set_rx_line_endings(esp_line_endings_t mode)
{
    _lock_acquire(&(s_vfstusb.read_lock));
    s_vfstusb.rx_mode = mode;
    _lock_release(&(s_vfstusb.read_lock));
}

void esp_vfs_tusb_cdc_set_tx_line_endings(esp_line_endings_t mode)
{
    _lock_acquire(&(s_vfstusb.write_lock));
    s_vfstusb.tx_mode = mode;
    _lock_release(&(s_vfstusb.write_lock));
}

#endif /* CONFIG_MM_IO_ENABLE_USB_OTG */
