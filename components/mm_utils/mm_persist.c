/**
 * @file
 *
 * @brief Utilities for handling object persistency into NVS
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_persist.h"

#include "nvs_flash.h"
#include "mm_err.h"

static nvs_handle_t s_nvs_handle = 0;
static int s_initialized = 0;

void mm_persist_init(void)
{
    esp_err_t ret;

    MM_CHECK_E(s_initialized == 0);

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ret = nvs_flash_erase();
        MM_CHECK_E(ret == ESP_OK);

        ret = nvs_flash_init();
    }
    MM_CHECK_E(ret == ESP_OK);

    ret = nvs_open("mm", NVS_READWRITE, &s_nvs_handle);
    MM_CHECK_E(ret == ESP_OK);

    s_initialized = 1;
    MM_LOG_I("Object nvs initialized");
}

mm_err_t mm_persist_store_obj(const char *key, const void *buf, size_t size)
{
    esp_err_t ret;
    ret = nvs_set_blob(s_nvs_handle, key, buf, size);
    if(ret == ESP_OK) {
        nvs_commit(s_nvs_handle);
        return MM_OK;
    }
    return MM_FAIL;
}

mm_err_t mm_persist_fetch_obj(const char *key, void *buf, size_t *size)
{
    esp_err_t ret;
    ret = nvs_get_blob(s_nvs_handle, key, buf, size);
    if(ret == ESP_OK) {
        return MM_OK;
    }
    return MM_FAIL;
}

mm_err_t mm_persist_invalidate_obj(const char *key)
{
    esp_err_t ret;
    ret = nvs_erase_key(s_nvs_handle, key);
    if(ret == ESP_OK) {
        nvs_commit(s_nvs_handle);
        return MM_OK;
    }
    return MM_FAIL;
}
