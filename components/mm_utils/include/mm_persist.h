/**
 * @file
 *
 * @brief Utilities for handling object persistency into NVS
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_PERSIST_H
#define __MM_PERSIST_H

#include <stddef.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    MM_PERSIST_COMPLEX_TYPE_STR,     /**< Holds char* (null-terminated string) */
    MM_PERSIST_COMPLEX_TYPE_BLOB,    /**< Holds generic buffer */
    MM_PERSIST_COMPLEX_TYPE_2D_STR,  /**< Holds char** as array of null-terminated strings */
} mm_persist_complex_field_type_t;

#define MM_PERSIST_COMPLEX_FIELD_STR(struct_type, ptr_member) \
    {MM_PERSIST_COMPLEX_TYPE_STR, offsetof(struct_type, ptr_member), 0, 0}

#define MM_PERSIST_COMPLEX_FIELD_BLOB(struct_type, ptr_member, len_member) \
    {MM_PERSIST_COMPLEX_TYPE_BLOB, offsetof(struct_type, ptr_member), offsetof(struct_type, len_member), 0}

#define MM_PERSIST_COMPLEX_FIELD_BLOB_FIXED(struct_type, ptr_member, len) \
    {MM_PERSIST_COMPLEX_TYPE_BLOB, offsetof(struct_type, ptr_member), 0, len}

#define MM_PERSIST_COMPLEX_FIELD_2D_STR(struct_type, ptr_member, count_member) \
    {MM_PERSIST_COMPLEX_TYPE_2D_STR, offsetof(struct_type, ptr_member), offsetof(struct_type, count_member), 0}

struct mm_persist_complex_field_s {
    mm_persist_complex_field_type_t type;
    size_t ptr_offset;
    size_t count_offset;
    size_t fixed_count;
};

struct mm_persist_complex_schema_s {
    size_t struct_size;
    size_t n_fields;
    const struct mm_persist_complex_field_s *fields;
};

void mm_persist_init(void);

mm_err_t mm_persist_store_obj(const char *key, const void *buf, size_t size);
mm_err_t mm_persist_fetch_obj(const char *key, void *buf, size_t *size);
void *mm_persist_fetch_obj_malloc(const char *key, size_t *size);
mm_err_t mm_persist_invalidate_obj(const char *key);

mm_err_t mm_persist_store_complex_obj(const char *key, const void *obj, const struct mm_persist_complex_schema_s *schema);
void *mm_persist_fetch_complex_obj_malloc(const char *key, const struct mm_persist_complex_schema_s *schema);
mm_err_t mm_persist_invalidate_complex_obj(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PERSIST_H */

