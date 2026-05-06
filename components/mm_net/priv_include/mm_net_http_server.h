/**
 * @file
 *
 * @brief HTTP server utilities
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_NET_HTTP_SERVER_H
#define __MM_NET_HTTP_SERVER_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C"
{
#endif

httpd_handle_t mm_net_http_server_start(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_NET_HTTP_SERVER_H */
