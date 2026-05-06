/**
 * @file
 *
 * @brief Network stack public functions
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_net.h"

#include <string.h>
#include "esp_wifi.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "mdns.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_err.h"
#include "mm_event.h"
#include "mm_types.h"
#include "mm_conf.h"
#include "mm_net_http_server.h"
#include "mm_net_sta.h"

static int s_initialized = 0;

static void mm_net_sntp_start(void);
static void mm_net_mdns_start(void);

static void __sntptrigger_handler(mm_event_t id, void *data)
{
    mm_net_sntp_sync();
}

static void __sethostname_handler_refresh_mdns(mm_event_t id, void *data)
{
    char *hostname = (char *) data;
    mdns_hostname_set(hostname);
    mm_conf_set(MM_CONF_HOSTNAME, hostname);
}

static void __setwifi_handler_update_credentials(mm_event_t id, void *data)
{
    struct mm_setwifi_param_s *param = (struct mm_setwifi_param_s *) data;
    mm_net_set_wifi_credentials(param->ssid, param->password);
}


void mm_net_init(void)
{
    MM_CHECK_E(s_initialized == 0);
    MM_CHECK_E(mm_conf_initialized());
    MM_CHECK_E(mm_initialized());

    mm_net_sta_start();
    mm_net_sntp_start();
    mm_net_mdns_start();
    mm_net_http_server_start();

    mm_event_add_handler(MM_EVENT_SETWIFI,       __setwifi_handler_update_credentials);
    mm_event_add_handler(MM_EVENT_SNTPTRIGGER,   __sntptrigger_handler);
    mm_event_add_handler(MM_EVENT_SETHOSTNAME,   __sethostname_handler_refresh_mdns);

    s_initialized = 1;
    MM_LOG_I("Network initialized");
}

int mm_net_initialized(void)
{
    return s_initialized;
}

void mm_net_set_wifi_credentials(const char *ssid, const char *password)
{
    esp_err_t ret;
    wifi_config_t wifi_config;

    /* get current config */
    ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    MM_CHECK_W(ret == ESP_OK);

    /* copy credentials into wifi config and NVS */
    if(ssid) {
        strncpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    }
    if(password) {
        strncpy((char *) wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }

    /* update wifi config */
    mm_net_sta_set_wifi_config(&wifi_config);

}

/* -------------------------- sntp */

static void __sntp_sync_cb(struct timeval *tv)
{
    mm_settime(tv);
}

static void mm_net_sntp_start(void)
{
    esp_err_t err;
    char *sntp_server = mm_conf_get_with_malloc(MM_CONF_NTP_SERVER);
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(sntp_server);

    config.sync_cb = __sntp_sync_cb;

    err = esp_netif_sntp_init(&config);
    MM_CHECK_E(err == ESP_OK);
}

void mm_net_sntp_sync(void)
{
    sntp_restart();
}

/* -------------------------- mdns */

static void mm_net_mdns_start(void)
{
    esp_err_t ret;
    char host_buf[64];
    size_t size = sizeof(host_buf);

    ret = mdns_init();
    MM_CHECK_E(ret == ESP_OK);

    mm_conf_get(MM_CONF_HOSTNAME, host_buf, &size);

    mdns_hostname_set(host_buf);

    //mdns_instance_name_set("Foo Bar Baz ESP32");
}

size_t mm_net_mdns_gethostname(const char *hostname, size_t size)
{
    mm_conf_get(MM_CONF_HOSTNAME, (void *) hostname, &size);
    return size;
}
