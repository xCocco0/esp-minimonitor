/**
 * @file
 *
 * @brief Network stack public functions
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_NET_H
#define __MM_NET_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void mm_net_init(void);
int mm_net_initialized(void);

void mm_net_set_wifi_credentials(const char *ssid, const char *password);

void mm_net_sntp_sync(void);

size_t mm_net_mdns_gethostname(const char *hostname, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __MM_NET_H */
