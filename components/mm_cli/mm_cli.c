/**
 * @file
 *
 * @brief CLI module public functions
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_cli_cmd.h"

#include <stddef.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include "freertos/FreeRTOS.h"
#include "linenoise/linenoise.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_console.h"
#include "esp_vfs_eventfd.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_event.h"
#include "mm_macros.h"
#include "mm_ringbuf.h"
#include "ansi_term_macros.h"

#define REPL_REQ_STOP 0x1
#define REPL_ACK_STOP 0x2

#define PROBE_TIMEOUT_MS 300
#define PROBE_RETRY_DELAY_MS 10

#define PROMPT_TEMPLATE(name) ""name"> "
#define PROMPT_PREFIX         PROMPT_TEMPLATE("%s")
#define PROMPT_PREFIX_COLOR   PROMPT_TEMPLATE(COLOR_STR(CONFIG_MM_CLI_PROMPT_COLOR)"%s"COLOR_STR(ANSI_RESET))

/* maximum size of prompt buffer (including terminator)
 * note: 2 is the size of %s in PROMPT_PREFIX */
#define PREFIX_MAX_LEN (CONFIG_MM_CLI_MAX_PROMPT_LEN + (MAX(sizeof(PROMPT_PREFIX_COLOR), sizeof(PROMPT_PREFIX)) - 2))

static int s_initialized = 0;
static char s_prompt_buf[PREFIX_MAX_LEN];
static EventGroupHandle_t s_repl_control_eg = NULL;
static TaskHandle_t s_repl_task_handle = NULL;
static int s_interrupt_reading_fd = -1;
static const uint64_t s_interrupt_reading_signal = 1;
static char s_stdin_rb_buf[64];
static mm_ringbuf_t s_stdin_rb = MM_RINGBUF_INIT(s_stdin_rb_buf, sizeof(s_stdin_rb_buf));

static void __stop_repl(void);
static void __start_repl(void);
void __repl_task(void *args);
static ssize_t __interruptible_read_generic(int fd, void *buf, size_t max_bytes, const struct timeval *timeout, int peek);
static inline ssize_t __interruptible_read(int fd, void *buf, size_t max_bytes);
static inline ssize_t __interruptible_read_peek(int fd, void *buf, size_t max_bytes, const struct timeval *timeout);
static inline ssize_t __interruptible_read_timed(int fd, void *buf, size_t max_bytes, const struct timeval *timeout);

/* override the default (weak) definition because it would
 * restore the linenoise read function with the default read() */
void linenoiseSetReadCharacteristics(void) {}

static void __cli_start_hook(mm_event_t id, void *data)
{
    __start_repl();
}
char *__cli_get_hint_wrapper(const char *buf, int *color, int *bold)
{
    const char* ret;
    ret = esp_console_get_hint(buf, color, bold);
    if(ret && !linenoiseIsDumbMode()) {
        *color = ANSI_BRIGHT_BLACK;
        *bold = 0;
    } else {
        *color = -1; // for linenoise -1 means don't put color code
        *bold = 0;
    }
    return (char *) ret;
}
static void __update_prompt_buf(void)
{
    char hostname_buf[CONFIG_MM_CLI_MAX_PROMPT_LEN+1];
    size_t size = sizeof(hostname_buf);
    mm_conf_get(MM_CONF_HOSTNAME, hostname_buf, &size);
    hostname_buf[CONFIG_MM_CLI_MAX_PROMPT_LEN] = '\0';
    if(linenoiseIsDumbMode()) {
        snprintf(s_prompt_buf, sizeof(s_prompt_buf), PROMPT_PREFIX, hostname_buf);
    } else {
        snprintf(s_prompt_buf, sizeof(s_prompt_buf), PROMPT_PREFIX_COLOR, hostname_buf);
    }
}

void mm_cli_init(void)
{
    MM_CHECK_E(s_initialized == 0);
    MM_CHECK_E(mm_conf_initialized());
    MM_CHECK_E(mm_initialized());
    MM_CHECK_E(!mm_started());

    s_repl_control_eg = xEventGroupCreate();
    MM_CHECK_E(s_repl_control_eg != NULL);

    esp_console_config_t config = ESP_CONSOLE_CONFIG_DEFAULT();
    config.max_cmdline_length = CONFIG_MM_CLI_MAX_LINE_LEN;
    config.max_cmdline_args = CONFIG_MM_CLI_MAX_ARG_NUM;
    #if CONFIG_SPIRAM
    config.heap_alloc_caps |= MALLOC_CAP_SPIRAM;
    #endif
    esp_console_init(&config);

    #if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    usb_serial_jtag_vfs_use_driver();
    usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
    #endif

    mm_cli_cmd_register_all();

    /* initialize linenoise */
    linenoiseSetMaxLineLen(CONFIG_MM_CLI_MAX_LINE_LEN);
    linenoiseHistorySetMaxLen(CONFIG_MM_CLI_HISTORY_SIZE);
    linenoiseSetCompletionCallback(esp_console_get_completion);
    linenoiseSetHintsCallback(__cli_get_hint_wrapper);

    /* start CLI task only after event queue is started */
    mm_event_add_handler(MM_EVENT_STARTUP, __cli_start_hook);

    s_initialized = 1;
    MM_LOG_I("CLI initialized");
}

static int __detect_ansi_term(void)
{
    const struct timeval timeout_zero = {.tv_sec = 0, .tv_usec = 0};
    size_t r;
    char *reply_code;
    /* should be long more than 4 bytes, because if the user pastes multiple commands we will
     * read the following commands first and then the report reply */
    static char probe_buf[64];

    fprintf(stdout, QUERY_DEVICE_STATUS_STR);
    fflush(stdout);
    fsync(fileno(stdout));

    for(int i = 0; i < PROBE_TIMEOUT_MS; i += PROBE_RETRY_DELAY_MS) {

        vTaskDelay(pdMS_TO_TICKS(PROBE_RETRY_DELAY_MS));

        r = __interruptible_read_peek(fileno(stdin), probe_buf, sizeof(probe_buf), &timeout_zero);

        /* find term report reply into probe_buf */
        reply_code = memmem(probe_buf, r, REPORT_DEVICE_OK_STR, sizeof(REPORT_DEVICE_OK_STR) - 1);
        if(reply_code) {
            return 1;
        }
    }
    /* timed out */
    return 0;
}

int mm_cli_initialized(void)
{
    return s_initialized;
}

static inline int __repl_select(int fd, fd_set *fds, struct timeval *timeout)
{
    fd_set fds_new;
    if(fds == NULL) {
        /* use temporary fd set */
        fds = &fds_new;
    }
    FD_ZERO(fds);
    FD_SET(fd, fds);
    if (s_interrupt_reading_fd != -1) {
        FD_SET(s_interrupt_reading_fd, fds);
    }
    int maxFd = MAX(fd, s_interrupt_reading_fd);
    return select(maxFd + 1, fds, NULL, NULL, timeout);
}

static ssize_t __interruptible_read_generic(int fd, void *buf, size_t max_bytes, const struct timeval *timeout, int peek)
{
    uint64_t signal_val = 0;
    int nread = 0;
    int streambuf_bytes = 0;
    fd_set read_fds;
    struct timeval timeout_copy;
    struct timeval *timeout_ptr;

    if (xEventGroupGetBits(s_repl_control_eg) & REPL_REQ_STOP) {
        return 0;
    }

    if(timeout) {
        /* we copy the timeout because we want to preserve the input param */
        timeout_copy = *timeout;
        timeout_ptr = &timeout_copy;
    } else {
        timeout_ptr = NULL;
    }

    /* if there is data on hold read it */
    if(peek) {
        streambuf_bytes = MM_RINGBUF_PEEK(&s_stdin_rb, buf, max_bytes);
    } else {
        streambuf_bytes = MM_RINGBUF_READ(&s_stdin_rb, buf, max_bytes);
    }

    if(streambuf_bytes >= max_bytes) {
        /* we have finished */
        return streambuf_bytes;
    }

    max_bytes -= streambuf_bytes;
    buf = (void *) ((char *) buf + streambuf_bytes);

    nread = __repl_select(fd, &read_fds, timeout_ptr);

    if (nread < 0) {
        /* error in select */
        return -1;
    }

    /* read termination request happened */
    if (FD_ISSET(s_interrupt_reading_fd, &read_fds)) {
        nread = read(s_interrupt_reading_fd, &signal_val, sizeof(signal_val));
        if ((nread != sizeof(signal_val)) || (signal_val != s_interrupt_reading_signal)) {
            /* e.g. multiple writes on eventfd */
            MM_LOG_W("Error reading from eventfd");
        }
        return streambuf_bytes;
    }

    /* bytes available for reading on fd */
    if (FD_ISSET(fd, &read_fds)) {
        /* call the read function with the number of bytes max_bytes */
        nread = read(fd, buf, max_bytes);
        /* append new data to stdin buffer */
        if(peek && nread > 0) {
            MM_RINGBUF_WRITE_BACK(&s_stdin_rb, buf, nread);
        }
    }

    /* timed out */
    if (!FD_ISSET(fd, &read_fds) && !FD_ISSET(s_interrupt_reading_fd, &read_fds)) {
        return 0;
    }

    return streambuf_bytes + nread;
}

static inline ssize_t __interruptible_read(int fd, void *buf, size_t max_bytes)
{
    return __interruptible_read_generic(fd, buf, max_bytes, NULL, 0);
}

static inline ssize_t __interruptible_read_peek(int fd, void *buf, size_t max_bytes, const struct timeval *timeout)
{
    return __interruptible_read_generic(fd, buf, max_bytes, timeout, 1);
}

static inline ssize_t __interruptible_read_timed(int fd, void *buf, size_t max_bytes, const struct timeval *timeout)
{
    return __interruptible_read_generic(fd, buf, max_bytes, timeout, 0);
}

void __repl_task(void *args)
{
    char *line;

    for(;;) {

        line = linenoise(s_prompt_buf);

        if(xEventGroupGetBits(s_repl_control_eg) & REPL_REQ_STOP) {
            /* we break the loop here because at this point we likely read gibberish */
            if(line) {
                linenoiseFree(line);
            }
            break;
        }

        if (line == NULL) {
            /* Ignore empty lines */
            continue;
        }

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if(err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if(err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if(err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if(err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }

        /* Add the command to the history */
        linenoiseHistoryAdd(line);
        /* Save command history to filesystem */
        // if (repl_com->history_save_path) {
        //     linenoiseHistorySave(repl_com->history_save_path);
        // }

        linenoiseFree(line);

        fprintf(stdout, QUERY_DEVICE_STATUS_STR);
        fflush(stdout);
        fsync(fileno(stdout));

        if(__detect_ansi_term()) {
            if(linenoiseIsDumbMode()) {
                MM_LOG_I("Switching to non-dumb terminal");
                linenoiseSetDumbMode(0);
                __update_prompt_buf();
                }
        } else {
            if(!linenoiseIsDumbMode()) {
                MM_LOG_I("Switching to dumb terminal");
                linenoiseSetDumbMode(1);
                __update_prompt_buf();
            }
        }
    }

    xEventGroupSetBits(s_repl_control_eg, REPL_ACK_STOP);
    vTaskDelete(NULL);
}

static void __start_repl(void)
{
    BaseType_t ret;
    /* we use non-blocking mode because we will wait in
     * the select call inside __interruptible_read */
    int stdin_fileno = fileno(stdin);
    int flags = fcntl(stdin_fileno, F_GETFL);
    flags |= O_NONBLOCK;
    (void)fcntl(stdin_fileno, F_SETFL, flags);

    esp_vfs_eventfd_config_t config = ESP_VFS_EVENTD_CONFIG_DEFAULT();
    MM_CHECK_E(esp_vfs_eventfd_register(&config) == ESP_OK);

    s_interrupt_reading_fd = eventfd(0, EFD_SUPPORT_ISR);
    MM_CHECK_E(s_interrupt_reading_fd != -1);

    linenoiseSetReadFunction(__interruptible_read);
    linenoiseSetDumbMode(!__detect_ansi_term());
    __update_prompt_buf();

    esp_console_register_help_command();

    MM_LOG_I("Starting REPL task");
    xEventGroupClearBits(s_repl_control_eg, REPL_REQ_STOP);
    ret = xTaskCreate(__repl_task, "mm_repl_task",
            configMINIMAL_STACK_SIZE + CONFIG_MM_CLI_TASK_ADDITIONAL_STACK,
            NULL,
            CONFIG_MM_CLI_TASK_PRIO,
            &s_repl_task_handle);
    MM_CHECK_E(ret == pdPASS);
}

static void __stop_repl(void)
{;
    xEventGroupSetBits(s_repl_control_eg, REPL_REQ_STOP);

    int nwrite = write(s_interrupt_reading_fd, &s_interrupt_reading_signal, sizeof(s_interrupt_reading_signal));
    if (nwrite != sizeof(s_interrupt_reading_signal)) {
        MM_CHECK_E(0);
        return;
    }

    xEventGroupWaitBits(s_repl_control_eg, REPL_ACK_STOP, pdTRUE, pdFALSE, portMAX_DELAY);
    MM_LOG_I("Stopped REPL task");

    s_repl_task_handle = NULL;
    close(s_interrupt_reading_fd);
    s_interrupt_reading_fd = -1;
    esp_console_deregister_help_command();
    esp_vfs_eventfd_unregister();
    linenoiseHistoryFree();
}
