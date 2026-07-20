/**
 * @file
 *
 * @brief WiFi station utilities
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_NET_STA_H
#define __MM_NET_STA_H

#include "esp_wifi.h"
#include "mm_err.h"

void mm_net_sta_set_wifi_config(wifi_config_t *wifi_config);
void mm_net_sta_start(void);
mm_err_t mm_wifi_getinfo(wifi_ap_record_t *ap_info, esp_netif_ip_info_t *ip_info);

#ifdef __cplusplus
}
#endif

#endif /* __MM_NET_STA_H */
