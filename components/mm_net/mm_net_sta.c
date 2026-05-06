/**
 * @file
 *
 * @brief WiFi station utilities
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_net_sta.h"

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_err.h"

#if CONFIG_MM_NET_WIFI_WPA3_SAE_PWE_HUNT_AND_PECK
    #define WIFI_SAE_MODE       WPA3_SAE_PWE_HUNT_AND_PECK
    #define H2E_IDENTIFIER      ""

#elif CONFIG_MM_NET_WIFI_WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define WIFI_SAE_MODE       WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define H2E_IDENTIFIER      CONFIG_MM_NET_WIFI_PW_ID

#elif CONFIG_MM_NET_WIFI_WPA3_SAE_PWE_BOTH
    #define WIFI_SAE_MODE       WPA3_SAE_PWE_BOTH
    #define H2E_IDENTIFIER      CONFIG_MM_NET_WIFI_PW_ID
#endif

#if CONFIG_MM_NET_WIFI_AUTH_OPEN
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

#elif CONFIG_MM_NET_WIFI_AUTH_WEP
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP

#elif CONFIG_MM_NET_WIFI_AUTH_WPA_PSK
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK

#elif CONFIG_MM_NET_WIFI_AUTH_WPA2_PSK
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

#elif CONFIG_MM_NET_WIFI_AUTH_WPA_WPA2_PSK
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

#elif CONFIG_MM_NET_WIFI_AUTH_WPA3_PSK
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK

#elif CONFIG_MM_NET_WIFI_AUTH_WPA2_WPA3_PSK
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK

#elif CONFIG_MM_NET_WIFI_AUTH_WAPI_PSK
    #define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_DISCONNECTED_BIT BIT1

#define WIFI_DISCONNECT_NOTIFY_ICON "wifi_slash"
#define WIFI_DISCONNECT_NOTIFY_NAME "wifi_disco"

static int s_initalized = 0;
static TimerHandle_t s_reconnect_timer;
static EventGroupHandle_t s_wifi_event_group;
static esp_netif_ip_info_t ipinfo;
static SemaphoreHandle_t s_connect_sema;


static void __connect_timer_cb(TimerHandle_t timer)
{
    if(xEventGroupGetBits(s_wifi_event_group) & WIFI_DISCONNECTED_BIT) {
        if(xSemaphoreTake(s_connect_sema, 0) == pdPASS) {
            esp_wifi_connect();
            xSemaphoreGive(s_connect_sema);
        }
    }
}

void mm_net_sta_set_wifi_config(wifi_config_t *wifi_config)
{
    esp_err_t err;

    xSemaphoreTake(s_connect_sema, portMAX_DELAY);

    /* fully stop wifi */
    err = esp_wifi_stop();
    MM_CHECK_W(err == ESP_OK);

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    xEventGroupClearBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);

    /* update config */
    err = esp_wifi_set_config(WIFI_IF_STA, wifi_config);
    MM_CHECK_W(err == ESP_OK);

    /* restart */
    err = esp_wifi_start();
    MM_CHECK_W(err == ESP_OK);

    xSemaphoreGive(s_connect_sema);
}

static int __default_wifi_config_if_changed(wifi_config_t *wifi_config)
{
    esp_err_t ret;
    int modified = 0;

    ret = esp_wifi_get_config(WIFI_IF_STA, wifi_config);
    if(!MM_CHECK_W(ret == ESP_OK)) {
        return 0;
    }

    if(wifi_config->sta.threshold.authmode != WIFI_SCAN_AUTH_MODE_THRESHOLD) {
        wifi_config->sta.threshold.authmode = WIFI_SCAN_AUTH_MODE_THRESHOLD;
        modified = 1;
    }
    if(wifi_config->sta.sae_pwe_h2e != WIFI_SAE_MODE) {
        wifi_config->sta.sae_pwe_h2e = WIFI_SAE_MODE;
        modified = 1;
    }
    if(strcmp((char *) wifi_config->sta.sae_h2e_identifier, H2E_IDENTIFIER) != 0) {
        strcpy((char *) wifi_config->sta.sae_h2e_identifier, H2E_IDENTIFIER);
        modified = 1;
    }

    if(wifi_config->sta.ssid[0] == 0) {
        /* we write something because an empty string doesn't cause WIFI_EVENT_STA_DISCONNECTED */
        wifi_config->sta.ssid[0] = '.';
        wifi_config->sta.ssid[1] = 0;
        modified = 1;
    }

    return modified;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT) {
        switch(event_id) {

        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            MM_LOG("Failed to connect to the AP");
            mm_notify(WIFI_DISCONNECT_NOTIFY_NAME, WIFI_DISCONNECT_NOTIFY_ICON,
                    0x0, 0, "Failed to connect to the AP", MM_NOTIFY_REPLACE, 1);
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
            break;

        default:
            /* other WIFI EVENT */
            break;
        }
    } else if(event_base == IP_EVENT) {
        switch(event_id) {

        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ipinfo = event->ip_info;
            MM_LOG("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            mm_snooze(WIFI_DISCONNECT_NOTIFY_NAME);
            xEventGroupClearBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;

        default:
            /* other IP EVENT */
            break;
        }
    }
}

void mm_net_sta_start(void)
{
    esp_err_t ret;
    BaseType_t retb;
    wifi_config_t wifi_config;

    if(!MM_CHECK_W(s_initalized == 0)) {
        return;
    }

    s_wifi_event_group = xEventGroupCreate();
    MM_CHECK_E(s_wifi_event_group != NULL);

    s_connect_sema = xSemaphoreCreateMutex();
    MM_CHECK_E(s_connect_sema != NULL);

    s_reconnect_timer = xTimerCreate("wifi_reconnect_timer", pdMS_TO_TICKS(CONFIG_MM_NET_RECONNECT_DELAY_MS), pdTRUE, 0, __connect_timer_cb);
    MM_CHECK_E(s_reconnect_timer != NULL);

    retb = xTimerStart(s_reconnect_timer, 0);
    MM_CHECK_E(retb == pdPASS);

    ret = esp_netif_init();
    MM_CHECK_E(ret == ESP_OK);

    ret = esp_event_loop_create_default();
    MM_CHECK_E(ret == ESP_OK);
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    MM_CHECK_E(ret == ESP_OK);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ret = esp_event_handler_instance_register(WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_id);
    MM_CHECK_E(ret == ESP_OK);
    ret = esp_event_handler_instance_register(IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            &instance_got_ip);
    MM_CHECK_E(ret == ESP_OK);

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    MM_CHECK_E(ret == ESP_OK);
    ret = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    MM_CHECK_E(ret == ESP_OK);
    if(__default_wifi_config_if_changed(&wifi_config)) {
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
    ret = esp_wifi_start();
    MM_CHECK_E(ret == ESP_OK);

    s_initalized = 1;
}
