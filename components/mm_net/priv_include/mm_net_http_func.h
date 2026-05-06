/**
 * @file
 *
 * @brief HTTP request handlers
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_NET_HTTP_FUNC_H
#define __MM_NET_HTTP_FUNC_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t mm_net_http_sensorconfig_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_setsensor_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_notify_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_snooze_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_index_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_favicon_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_panic_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_coredump_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_setconfig_get_handler(httpd_req_t *req);
esp_err_t mm_net_http_ws_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* __MM_NET_HTTP_FUNC_H */
