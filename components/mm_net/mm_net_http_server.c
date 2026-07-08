/**
 * @file
 *
 * @brief HTTP server utilities
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_net_http_server.h"

#include "esp_system.h"
#include "esp_http_server.h"
#include "sdkconfig.h"
#include "mm_err.h"
#include "mm_event.h"
#include "mm_macros.h"
#include "mm_net_http_func.h"

static httpd_handle_t server = NULL;

static const httpd_uri_t httpd_uri_list[] = {
    {
        .uri       = "/setsensor",
        .method    = HTTP_GET,
        .handler   = mm_net_http_setsensor_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/sensorconfig",
        .method    = HTTP_GET,
        .handler   = mm_net_http_sensorconfig_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/notify",
        .method    = HTTP_GET,
        .handler   = mm_net_http_notify_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/snooze",
        .method    = HTTP_GET,
        .handler   = mm_net_http_snooze_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/panic",
        .method    = HTTP_GET,
        .handler   = mm_net_http_panic_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/coredump.bin",
        .method    = HTTP_GET,
        .handler   = mm_net_http_coredump_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/setconfig",
        .method    = HTTP_GET,
        .handler   = mm_net_http_setconfig_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = mm_net_http_index_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/index.html",
        .method    = HTTP_GET,
        .handler   = mm_net_http_index_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/favicon.ico",
        .method    = HTTP_GET,
        .handler   = mm_net_http_favicon_get_handler,
        .user_ctx  = NULL
    }, {
        .uri       = "/ws",
        .method    = HTTP_GET,
        .handler   = mm_net_http_ws_handler,
        .user_ctx  = NULL,
        .is_websocket = true
    }
};

static esp_err_t __ws_send_all_clients(httpd_handle_t server_handle, httpd_ws_frame_t *ws_pkt)
{
    const size_t max_clients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t fds = max_clients;
    int client_fds[max_clients];

    esp_err_t ret = httpd_get_client_list(server_handle, &fds, client_fds);

    if (ret != ESP_OK) {
        return ret;
    }

    for (int i = 0; i < fds; i++) {
        httpd_ws_client_info_t client_info = httpd_ws_get_fd_info(server_handle, client_fds[i]);
        if(client_info == HTTPD_WS_CLIENT_WEBSOCKET && httpd_sess_get_ctx(server, client_fds[i]) == (void*) 1) {
            httpd_ws_send_frame_async(server_handle, client_fds[i], ws_pkt);
        }
    }

    return ESP_OK;
}

static uint32_t __mm_pow(uint32_t a, int b) // TODO: move to mm_utils
{
    uint32_t ret = 1;
    for(int i = 0; i < b; ++i) {
        ret *= a;
    }
    return ret;
}

static void __mm_net_http_ws_send_sensor(mm_event_t id, void *data)
{
    char value_display[CONFIG_MM_SENSOR_DESC_FMT_MAX_LEN];
    uint32_t integer_part, decimal_part, color_v;
    struct mm_sensor_data_s *param = (struct mm_sensor_data_s *) data;
    struct mm_sensor_meter_config_s conf;
    uint32_t color;

    /* fetch config from nvs */
    conf.index = param->index;
    if(mm_sensor_fetch_meter_config(&conf) != MM_OK) {
        MM_LOG_W("Sensor %d is not configured yet, skipping", param->index);
        return;
    }

    if(server == NULL) {
        return;
    }

    /* get bar color */
    color_v = CHOOSE3(conf.color_arg, param->value1, param->value2, param->valueP);
    color = color_v < conf.threshold.low ? conf.color.base : (
            color_v < conf.threshold.high ? conf.color.warn :
            conf.color.alert);

    /* value string to show */
    if(conf.decimals > 0) {
        integer_part = param->value1 / __mm_pow(10, conf.decimals);
        decimal_part = param->value1 % __mm_pow(10, conf.decimals);
        snprintf(value_display, sizeof(value_display), conf.fmt, MM_SENSOR_PRINTF_ARGS(integer_part, decimal_part, conf.decimals, param->value2));
    } else {
        snprintf(value_display, sizeof(value_display), conf.fmt, MM_SENSOR_PRINTF_ARGS(param->value1, 0, 0, param->value2));
    }

    /* format json string */
    char json_buf[160];
    int len = snprintf(json_buf, sizeof(json_buf),
                      "{\"index\": %d, \"value\": %"PRIu32", \"value_display\": \"%s\", \"bar_color\": \"#%06"PRIx32"\"}",
                      param->index, param->value1, value_display, color);

    /* send to web socket */
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)json_buf;
    ws_pkt.len = MIN(len, sizeof(json_buf) - 1);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    __ws_send_all_clients(server, &ws_pkt);
}

httpd_handle_t mm_net_http_server_start(void)
{
    esp_err_t ret;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.lru_purge_enable = true;
    config.server_port = CONFIG_MM_NET_HTTP_SERVER_PORT;
    config.max_uri_handlers = sizeof(httpd_uri_list)/sizeof(httpd_uri_list[0]);

    ret = httpd_start(&server, &config);
    MM_CHECK_E(ret == ESP_OK);
    MM_CHECK_E(server != NULL);

    for(int i = 0; i < sizeof(httpd_uri_list)/sizeof(httpd_uri_list[0]); ++i) {
        ret = httpd_register_uri_handler(server, &httpd_uri_list[i]);
        MM_CHECK_E(ret == ESP_OK);
    }

    mm_event_add_handler(MM_EVENT_SETSENSOR, __mm_net_http_ws_send_sensor);

    return server;
}
