/**
 * @file
 *
 * @brief Handle system configuration variables
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_conf.h"

#include <stdint.h>
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "mm_err.h"
#include "mm_macros.h"
#include "mm_conf_macros.h"


static int s_initialized = 0;
static nvs_handle_t config_nvs_handle = 0;
static SemaphoreHandle_t s_config_mutex = NULL;
static esp_timer_handle_t nvs_commit_timer = NULL;

union multitype_u {
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    char* str;
};

struct config_element_s {
    const char *name;
    const char *key;
    const mm_conf_type_t type;
    const union multitype_u default_value;
    void *startup_value;
    int requires_reboot : 1;
};

static struct config_element_s config[] = {
    #define CONFIG_FOR_EACH_ENTRY(name, key, type, default_value, req_reb) CONFIG_DEF_ENTRY(name, key, type, default_value, req_reb),
    #include "mm_conf_entries.def"
    #undef CONFIG_FOR_EACH_ENTRY
};

#define __CHECK_TYPE(type, value, size, call) \
    if(size == NULL) { \
        return ESP_ERR_NVS_INVALID_LENGTH; \
    } else if(value == NULL) { \
        *size = sizeof(type); \
        return ESP_OK; \
    } else if(*size < sizeof(type)) { \
        *size = sizeof(type); \
        return ESP_ERR_NVS_INVALID_LENGTH; \
    } else { \
        return call; \
    }

static inline esp_err_t __nvs_get_by_type(mm_conf_type_t type, const char *key, void *value, size_t *size) {
    switch(type) {
        case MM_CONF_TYPE_I8:  __CHECK_TYPE(int8_t,   value, size, nvs_get_i8( config_nvs_handle, key,   (int8_t *) value));
        case MM_CONF_TYPE_U8:  __CHECK_TYPE(uint8_t,  value, size, nvs_get_u8( config_nvs_handle, key,  (uint8_t *) value));
        case MM_CONF_TYPE_I16: __CHECK_TYPE(int16_t,  value, size, nvs_get_i16(config_nvs_handle, key,  (int16_t *) value));
        case MM_CONF_TYPE_U16: __CHECK_TYPE(uint16_t, value, size, nvs_get_u16(config_nvs_handle, key, (uint16_t *) value));
        case MM_CONF_TYPE_I32: __CHECK_TYPE(int32_t,  value, size, nvs_get_i32(config_nvs_handle, key,  (int32_t *) value));
        case MM_CONF_TYPE_U32: __CHECK_TYPE(uint32_t, value, size, nvs_get_u32(config_nvs_handle, key, (uint32_t *) value));
        case MM_CONF_TYPE_STR: return nvs_get_str(config_nvs_handle, key,     (char *) value, size);
        default:               return ESP_FAIL;
    }
    MM_CHECK_E(0);
}

static inline esp_err_t __nvs_set_by_type(mm_conf_type_t type, const char *key, const void *value) {
    switch(type) {
        case MM_CONF_TYPE_I8:  return nvs_set_i8( config_nvs_handle, key, *(  (int8_t *) value)); break;
        case MM_CONF_TYPE_U8:  return nvs_set_u8( config_nvs_handle, key, *( (uint8_t *) value)); break;
        case MM_CONF_TYPE_I16: return nvs_set_i16(config_nvs_handle, key, *( (int16_t *) value)); break;
        case MM_CONF_TYPE_U16: return nvs_set_u16(config_nvs_handle, key, *((uint16_t *) value)); break;
        case MM_CONF_TYPE_I32: return nvs_set_i32(config_nvs_handle, key, *( (int32_t *) value)); break;
        case MM_CONF_TYPE_U32: return nvs_set_u32(config_nvs_handle, key, *((uint32_t *) value)); break;
        case MM_CONF_TYPE_STR: return nvs_set_str(config_nvs_handle, key,       (char *) value ); break;
        default:               return ESP_FAIL;                                                   break;
    }
    MM_CHECK_E(0);
}

static void __nvs_commit_timer_cb(void *arg)
{
    /* this is useless but the documentation says to add it... */
    nvs_commit(config_nvs_handle);
}

static mm_err_t __mm_conf_get_internal(mm_conf_entry_t entry, void *value, size_t *size);

void mm_conf_init(void)
{
    esp_err_t ret;
    size_t size;
    void *ptr;

    MM_CHECK_E(s_initialized == 0);

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ret = nvs_flash_erase();
        MM_CHECK_E(ret == ESP_OK);

        ret = nvs_flash_init();
    }
    MM_CHECK_E(ret == ESP_OK);

    ret = nvs_open("mm", NVS_READWRITE, &config_nvs_handle);
    MM_CHECK_E(ret == ESP_OK);

    s_config_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_config_mutex == NULL);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &__nvs_commit_timer_cb,
        .name = "mm_config_nvs_update",
    };

    ret = esp_timer_create(&periodic_timer_args, &nvs_commit_timer);
    MM_CHECK_E(ret == ESP_OK);

    ret = esp_timer_start_periodic(nvs_commit_timer, CONFIG_MM_UTILS_CONF_REFRESH_INTERVAL_MS*1000);
    MM_CHECK_E(ret == ESP_OK);

    for(int i = 0; i < MM_CONF_SIZE; ++i) {
        if(config[i].requires_reboot) {
            if(config[i].type == MM_CONF_TYPE_STR) {
                /* malloc the string and copy pointer to config buffer */
                __mm_conf_get_internal(i, NULL, &size);
                ptr = malloc(size);
                MM_CHECK_MALLOC_E(ptr);
                __mm_conf_get_internal(i, ptr, &size);
                memcpy((void *) &config[i].startup_value, &ptr, sizeof(void*));
            } else {
                /* simply copy value to config buffer */
                size = sizeof(config[i].startup_value);
                __mm_conf_get_internal(i, &config[i].startup_value, &size);
            }
        }
    }

    s_initialized = 1;
    MM_LOG_I("Config initialized");
}

int mm_conf_initialized(void)
{
    return s_initialized;
}

mm_err_t mm_conf_set(mm_conf_entry_t entry, const void *value)
{
    esp_err_t err;

    if(!MM_CHECK_W(s_initialized == 1)) {
        return MM_FAIL;
    };

    if(!MM_CHECK_W(entry < MM_CONF_SIZE)) {
        return MM_FAIL;
    }

    (void) xSemaphoreTake(s_config_mutex, portMAX_DELAY);

    err = __nvs_set_by_type(config[entry].type, config[entry].key, value);

    (void) xSemaphoreGive(s_config_mutex);

    return err == ESP_OK ? MM_OK : MM_FAIL;
}

mm_err_t mm_conf_get(mm_conf_entry_t entry, void *value, size_t *size)
{
    if(!MM_CHECK_W(s_initialized == 1)) {
        return MM_FAIL;
    };

    if(!MM_CHECK_W(entry < MM_CONF_SIZE)) {
        return MM_FAIL;
    }

    /* the value is buffered only after mm_conf_init finished
     * if not initialized continue normally and read from nvs */
    if(config[entry].requires_reboot) {
        if(config[entry].type == MM_CONF_TYPE_STR) {
            if(value) {
                *size = MIN(*size, 1+strlen(config[entry].startup_value));
                memcpy(value, config[entry].startup_value, *size);
                /* add a terminator in case the destination buffer was too small */
                ((char *) value)[*size-1] = '\0';
            } else {
                *size = 1+strlen(config[entry].startup_value);
            }
        } else {
            if(value) {
                /* TODO: replace void* with correct size */
                *size = MIN(*size, sizeof(void*));
                memcpy(value, &config[entry].startup_value, *size);
            } else {
                *size = sizeof(void*);
            }
        }
        return MM_OK;
    }

    return __mm_conf_get_internal(entry, value, size);
}

static mm_err_t __mm_conf_get_internal(mm_conf_entry_t entry, void *value, size_t *size)
{
    esp_err_t err;
    void *default_v;

    (void) xSemaphoreTake(s_config_mutex, portMAX_DELAY);

    err = __nvs_get_by_type(config[entry].type, config[entry].key, value, size);

    if(err == ESP_ERR_NVS_NOT_FOUND) {

        if(config[entry].type == MM_CONF_TYPE_STR) {
            /* copy the string pointer */
            default_v = (void *) config[entry].default_value.str;
        } else {
            /* use the pointer to the struct member */
            default_v = (void *) &config[entry].default_value;
        }

        err = __nvs_set_by_type(config[entry].type, config[entry].key, default_v);
        if(!MM_CHECK_W(err == ESP_OK)) {
            return MM_FAIL;
        }

        err = __nvs_get_by_type(config[entry].type, config[entry].key, value, size);
    }
    if(!MM_CHECK_W(err == ESP_OK)) {
        return MM_FAIL;
    }

    (void) xSemaphoreGive(s_config_mutex);

    return MM_OK;
}

uint32_t mm_conf_get_as_integer(mm_conf_entry_t entry)
{
    uint32_t buf = 0;
    size_t size = sizeof(buf);

    if(!MM_CHECK_W(entry < MM_CONF_SIZE)) {
        return MM_FAIL;
    }

    if(config[entry].type == MM_CONF_TYPE_STR) {
        return 0;
    }

    if(!MM_CHECK_W(mm_conf_get(entry, &buf, &size) == MM_OK)) {
        return 0;
    }

    return buf;
}

void *mm_conf_get_with_malloc(mm_conf_entry_t entry)
{
    void *buf;
    size_t size;

    if(!MM_CHECK_W(mm_conf_get(entry, NULL, &size) == MM_OK)) {
        return NULL;
    }

    buf = malloc(size);
    if(!MM_CHECK_MALLOC_W(buf)) {
        return NULL;
    }

    if(!MM_CHECK_W(mm_conf_get(entry, buf, &size)  == MM_OK)) {
        free(buf);
        return NULL;
    }

    return buf;
}

mm_err_t mm_conf_get_as_string(mm_conf_entry_t entry, char *buf, size_t *size)
{
    mm_conf_type_t type = mm_conf_get_type(entry);
    union multitype_u val;
    size_t var_size, str_size;
    mm_err_t err;

    #define __ENTRY_TO_STR_BY_TYPE(type, format) {\
            var_size = sizeof(val.type); \
            err = mm_conf_get(entry, &val.type, &var_size); \
            str_size = snprintf(buf, buf ? *size : 0, format, val.type); \
            *size = 1+str_size; \
    }

    switch(type) {
        case MM_CONF_TYPE_I8:  __ENTRY_TO_STR_BY_TYPE(i8,  "%"PRIi8"");  break;
        case MM_CONF_TYPE_U8:  __ENTRY_TO_STR_BY_TYPE(u8,  "%"PRIu8"");  break;
        case MM_CONF_TYPE_I16: __ENTRY_TO_STR_BY_TYPE(i16, "%"PRIi16""); break;
        case MM_CONF_TYPE_U16: __ENTRY_TO_STR_BY_TYPE(u16, "%"PRIu16""); break;
        case MM_CONF_TYPE_I32: __ENTRY_TO_STR_BY_TYPE(i32, "%"PRIi32""); break;
        case MM_CONF_TYPE_U32: __ENTRY_TO_STR_BY_TYPE(u32, "%"PRIu32""); break;
        case MM_CONF_TYPE_STR: err = mm_conf_get(entry, buf, size); break;
        default:
            return MM_FAIL;
    }

    #undef __ENTRY_TO_STR_BY_TYPE

    return err;
}

mm_err_t mm_conf_set_from_string(mm_conf_entry_t entry, const char *buf)
{
    mm_conf_type_t type;
    union multitype_u v;
    int base = 10;
    mm_err_t err;

    type = mm_conf_get_type(entry);

    if(buf[0] == '0') {
        if(buf[1] == 'x') {
            base = 16;
        } else {
            base = 8;
        }
    }

    #define __CMD_CONFIG_BY_TYPE(type, fmt) do { \
        char format[] = fmt; \
        if(base ==  8) format[sizeof(fmt)-2] = 'o'; \
        if(base == 16) format[sizeof(fmt)-2] = 'x'; \
        sscanf(buf, format, &v.type); \
        err = mm_conf_set(entry, &v.type); \
    } while(0)

    switch(type) {
        case MM_CONF_TYPE_I8:  __CMD_CONFIG_BY_TYPE(i8, "%"PRIi8""); break;
        case MM_CONF_TYPE_U8:  __CMD_CONFIG_BY_TYPE(u8, "%"PRIu8""); break;
        case MM_CONF_TYPE_I16: __CMD_CONFIG_BY_TYPE(i16, "%"PRIi16""); break;
        case MM_CONF_TYPE_U16: __CMD_CONFIG_BY_TYPE(u16, "%"PRIu16""); break;
        case MM_CONF_TYPE_I32: __CMD_CONFIG_BY_TYPE(i32, "%"PRIi32""); break;
        case MM_CONF_TYPE_U32: __CMD_CONFIG_BY_TYPE(u32, "%"PRIu32""); break;
        case MM_CONF_TYPE_STR:
            err = mm_conf_set(entry, buf);
            break;
        default:
            return MM_FAIL;
    }

    #undef __CMD_CONFIG_BY_TYPE

    return err;
}

mm_err_t mm_conf_restore_default(mm_conf_entry_t entry)
{
    if(mm_conf_get_type(entry) == MM_CONF_TYPE_STR) {
        return mm_conf_set(entry, config[entry].default_value.str);
    } else {
        return mm_conf_set(entry, (void *) &config[entry].default_value);
    }
}

mm_err_t mm_conf_restore_all_defaults(void)
{
    for(int i = 0; i < MM_CONF_SIZE; ++i) {
        if(mm_conf_restore_default(i) == MM_FAIL) {
            return MM_FAIL;
        };
    }
    return MM_OK;
}


mm_conf_entry_t mm_conf_entry_by_name(const char *name)
{
    for(int i = 0; i < MM_CONF_SIZE; ++i) {
        if(strcmp(config[i].name, name) == 0) {
            return (mm_conf_entry_t) i;
        }
    }
    return MM_CONF_NONE;
}

int mm_conf_requires_reboot(mm_conf_entry_t entry)
{
    if(!MM_CHECK_W(entry < MM_CONF_SIZE)) {
        return MM_CONF_NONE;
    }

    return config[entry].requires_reboot;
}

mm_conf_type_t mm_conf_get_type(mm_conf_entry_t entry)
{
    if(!MM_CHECK_W(entry < MM_CONF_SIZE)) {
        return MM_CONF_NONE;
    }

    return config[entry].type;
}

const char *mm_conf_get_name(mm_conf_entry_t entry)
{
    if(!MM_CHECK_W(entry < MM_CONF_SIZE)) {
        return "";
    }

    return config[entry].name;
}
