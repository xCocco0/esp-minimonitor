/**
 * @file
 *
 * @brief Utilities for handling object persistency into NVS
 *
 * Serialization schema:
 *
 * ```
 * nvs buffer
 *  |
 *  +-> main_object [struct_size bytes]
 *  |
 *  +-> additional_fields --+-> binary blob ------+-> full_size [4 bytes]
 *                          |                     |
 *                          |                     +-> data [full_size]
 *                          |
 *                          +-> single string ----+-> full_size (includes \0 terminator) [4 bytes]
 *                          |                     |
 *                          |                     +-> data [full_size]
 *                          |
 *                          +-> 2d string array --+-> count [4 bytes]
 *                                                |
 *                                                +-> full_size_1 (includes \0 terminator) [4 bytes]
 *                                                +-> full_size_2 [4 bytes]
 *                                                +-> ...
 *                                                +-> full_size_count [4 bytes]
 *                                                |
 *                                                +-> data_1 [full_size_1] bytes]
 *                                                +-> data_2 [full_size_2] bytes]
 *                                                +-> ...
 *                                                +-> data_count [full_size_count] bytes]
 * ```
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_persist.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "nvs_flash.h"
#include "mm_err.h"

/* round up x to be a multiple of align */
#define ROUND_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

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
    /* note: the nvs library will detect if the item was modified,
             no need to check again here */
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

void *mm_persist_fetch_obj_malloc(const char *key, size_t *size)
{
    esp_err_t ret;
    void *buf;
    size_t s;
    ret = nvs_get_blob(s_nvs_handle, key, NULL, &s);
    if(ret != ESP_OK || s == 0) {
        if(size) {
            *size = 0;
        }
        /* not found */
        return NULL;
    }

    buf = malloc(s);
    MM_CHECK_MALLOC_E(buf);

    ret = nvs_get_blob(s_nvs_handle, key, buf, &s);
    if(ret != ESP_OK) {
        if(size) {
            *size = 0;
        }
        return NULL;
    }

    if(size) {
        *size = s;
    }

    return buf;
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

void __clear_pointers(void *obj, const struct mm_persist_complex_schema_s *schema)
{
    uint8_t *base;
    size_t i;
    const struct mm_persist_complex_field_s *f;

    if(!obj || !schema) {
        return;
    }

    base = (uint8_t *)obj;

    for(i = 0; i < schema->n_fields; i++) {
        f = &schema->fields[i];
        *(void **)(base + f->ptr_offset) = NULL;
    }
}

static size_t __store_complex(uint8_t *buf, const uint8_t *obj, const struct mm_persist_complex_schema_s *schema)
{
    size_t total_size;
    const struct mm_persist_complex_field_s *f;
    void *ptr;
    char **arr;
    uint8_t *p;
    uint32_t u32_len, u32_count, str_len;
    uint8_t *len_table;

    total_size = schema->struct_size;

    if(buf) {
        memcpy(buf, obj, schema->struct_size);
        p = buf + schema->struct_size;
        __clear_pointers(buf, schema);
    } else {
        p = NULL;
    }

    for(size_t i = 0; i < schema->n_fields; i++) {
        f = &schema->fields[i];
        ptr = *(void **)(obj + f->ptr_offset);

        if(f->type == MM_PERSIST_COMPLEX_TYPE_STR) {
            u32_len = ptr ? (uint32_t)(strlen((const char *) ptr) + 1) : 0;
            total_size += sizeof(uint32_t) + u32_len;
            if(buf) {
                memcpy(p, &u32_len, sizeof(u32_len));
                p += sizeof(u32_len);
                if(u32_len > 0) {
                    memcpy(p, ptr, u32_len);
                    p += u32_len;
                }
            }
        } else if(f->type == MM_PERSIST_COMPLEX_TYPE_BLOB) {
            u32_len = f->count_offset ? (uint32_t) *(size_t *)(obj + f->count_offset) : (uint32_t) f->fixed_count;
            if(!ptr) {
                u32_len = 0;
            }
            total_size += sizeof(uint32_t) + u32_len;
            if(buf) {
                memcpy(p, &u32_len, sizeof(u32_len));
                p += sizeof(u32_len);
                if(u32_len > 0) {
                    memcpy(p, ptr, u32_len);
                    p += u32_len;
                }
            }
        } else if(f->type == MM_PERSIST_COMPLEX_TYPE_2D_STR) {
            u32_count = f->count_offset ? (uint32_t) *(size_t *)(obj + f->count_offset) : (uint32_t) f->fixed_count;
            total_size += sizeof(uint32_t) + (u32_count * sizeof(uint32_t));

            arr = ptr ? (char **) ptr : NULL;

            if(buf) {
                memcpy(p, &u32_count, sizeof(u32_count));
                p += sizeof(u32_count);
                len_table = p;
                p += u32_count * sizeof(uint32_t);
            } else {
                len_table = NULL;
            }

            for(uint32_t c = 0; c < u32_count; c++) {
                str_len = (arr && arr[c]) ? (uint32_t)(strlen(arr[c]) + 1) : 0;
                total_size += str_len;
                if(buf) {
                    memcpy(len_table + (c * sizeof(uint32_t)), &str_len, sizeof(str_len));
                    if(str_len > 0) {
                        memcpy(p, arr[c], str_len);
                        p += str_len;
                    }
                }
            }
        }
    }

    return total_size;
}

mm_err_t mm_persist_store_complex_obj(const char *key, const void *obj, const struct mm_persist_complex_schema_s *schema)
{
    const uint8_t *base;
    size_t total_size;
    uint8_t *buf;
    int ret;

    if(!MM_CHECK_W(key && obj && schema)) {
        return MM_FAIL;
    }

    base = (const uint8_t *) obj;

    /* calculate total size */
    total_size = __store_complex(NULL, base, schema);

    buf = (uint8_t *) malloc(total_size);
    MM_CHECK_MALLOC_E(buf);

    /* serialize data into buffer */
    __store_complex(buf, base, schema);

    ret = mm_persist_store_obj(key, buf, total_size);
    free(buf);

    return ret;
}

static size_t __fetch_complex(uint8_t *obj, const uint8_t *payload, const struct mm_persist_complex_schema_s *schema)
{
    size_t obj_size;
    const struct mm_persist_complex_field_s *f;
    uint32_t len, count, expected_count;
    const uint32_t *lens;
    const uint8_t *p;
    uint8_t *obj_cursor;
    char **ptr_table = NULL;

    obj_size = ROUND_UP(schema->struct_size, sizeof(void *));
    p = payload;

    if(obj) {
        obj_cursor = obj + obj_size;
    } else {
        obj_cursor = NULL;
    }

    for(size_t i = 0; i < schema->n_fields; i++) {
        f = &schema->fields[i];

        if(f->type == MM_PERSIST_COMPLEX_TYPE_STR || f->type == MM_PERSIST_COMPLEX_TYPE_BLOB) {
            memcpy(&len, p, sizeof(len));
            p += sizeof(len);
            if(len > 0) {
                if(obj) {
                    *(void **)(obj + f->ptr_offset) = (void *) obj_cursor;
                    memcpy(obj_cursor, p, len);
                    obj_cursor += ROUND_UP(len, sizeof(void *));
                }
                p += len;
                obj_size += ROUND_UP(len, sizeof(void *));
            } else {
                if(obj) {
                    *(void **)(obj + f->ptr_offset) = NULL;
                }
            }
        } else if(f->type == MM_PERSIST_COMPLEX_TYPE_2D_STR) {
            memcpy(&count, p, sizeof(count));
            p += sizeof(count);

            if(obj) {
                expected_count = f->count_offset ? *(size_t *)(obj + f->count_offset) : f->fixed_count;
                if(count != expected_count) {
                    MM_LOG_W("Items in NVS (%"PRIu32") do not match the main struct count member (%u).", count, expected_count);
                }
            }

            if(count > 0) {
                if(obj) {
                    ptr_table = (char **)obj_cursor;
                    *(char ***)(obj + f->ptr_offset) = ptr_table;
                    obj_cursor += ROUND_UP(count * sizeof(char *), sizeof(void *));
                }
                obj_size += ROUND_UP(count * sizeof(char *), sizeof(void *));

                lens = (const uint32_t *) p;
                p += count * sizeof(uint32_t);

                for(size_t c = 0; c < count; c++) {
                    if(lens[c] > 0) {
                        if(obj) {
                            ptr_table[c] = (char *) obj_cursor;
                            memcpy(obj_cursor, p, lens[c]);
                            obj_cursor += ROUND_UP(lens[c], sizeof(void *));
                        }
                        p += lens[c];
                        obj_size += ROUND_UP(lens[c], sizeof(void *));
                    } else {
                        if(obj) {
                            ptr_table[c] = NULL;
                        }
                    }
                }
            } else {
                if(obj) {
                    *(void **)(obj + f->ptr_offset) = NULL;
                }
            }
        }
    }

    return obj_size;
}

void *mm_persist_fetch_complex_obj_malloc(const char *key, const struct mm_persist_complex_schema_s *schema)
{
    size_t nvs_size;
    uint8_t *buf;
    const uint8_t *payload;
    size_t obj_size;
    uint8_t *obj;

    if(!MM_CHECK_W(key && schema)) {
        return NULL;
    }

    buf = mm_persist_fetch_obj_malloc(key, &nvs_size);
    if(!buf) {
        /* not found */
        return NULL;
    } else if(nvs_size < schema->struct_size) {
        MM_LOG_W("Size mismatch while reading object from NVS");
        free(buf);
        return NULL;
    }

    payload = buf + schema->struct_size;

    /* calculate size */
    obj_size = __fetch_complex(NULL, payload, schema);

    /* allocate buffer */
    obj = (uint8_t *) calloc(1, obj_size);
    MM_CHECK_MALLOC_E(obj);

    /* copy obj struct into object buffer */
    memcpy(obj, buf, schema->struct_size);

    /* copy additional data */
    __fetch_complex(obj, payload, schema);

    free(buf);
    return obj;
}

mm_err_t mm_persist_invalidate_complex_obj(const char *key)
{
    return mm_persist_invalidate_obj(key);
}
