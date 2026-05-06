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

void mm_net_sta_set_wifi_config(wifi_config_t *wifi_config);
void mm_net_sta_start(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_NET_STA_H */
