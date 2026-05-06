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
#include "esp_console.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_event.h"

static int s_initialized = 0;
static char s_prompt_buf[CONFIG_MM_CLI_MAX_PROMPT_LEN+2]; /* prompt + '>' + '\0' */
static esp_console_repl_t *s_repl = NULL;

static void __cli_start_hook(mm_event_t id, void *data)
{
    ESP_ERROR_CHECK(esp_console_start_repl(s_repl));
}

void mm_cli_init(void)
{
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    char hostname_buf[CONFIG_MM_CLI_MAX_PROMPT_LEN+1];
    size_t size = sizeof(hostname_buf);

    MM_CHECK_E(s_initialized == 0);
    MM_CHECK_E(mm_conf_initialized());
    MM_CHECK_E(mm_initialized());
    MM_CHECK_E(!mm_started());

    mm_conf_get(MM_CONF_HOSTNAME, hostname_buf, &size);
    hostname_buf[CONFIG_MM_CLI_MAX_PROMPT_LEN+1] = '\0';

    snprintf(s_prompt_buf, sizeof(s_prompt_buf), "%s>", hostname_buf);

    repl_config.prompt = s_prompt_buf;
    repl_config.max_cmdline_length = CONFIG_MM_CLI_LINE_BUF_SIZE;

    esp_console_register_help_command();

    mm_cli_cmd_register_all();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &s_repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &s_repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &s_repl));
#else
#error Unsupported console type
#endif

    /* start CLI task only after event queue is started */
    mm_event_add_handler(MM_EVENT_STARTUP, __cli_start_hook);

    s_initialized = 1;
    MM_LOG_I("CLI initialized");
}

int mm_cli_initialized(void)
{
    return s_initialized;
}
