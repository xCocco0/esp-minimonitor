/**
 * @file
 *
 * @brief Handle system configuration variables
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_CONF_H
#define __MM_CONF_H

#include <stddef.h>
#include <stdint.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* CONFIG_xxx definitions */
typedef enum {
    MM_CONF_NONE = -1,
    #define CONFIG_FOR_EACH_ENTRY(name, ...) MM_CONF_##name ,
    #include "mm_conf_entries.def"
    #undef CONFIG_FOR_EACH_ENTRY
    MM_CONF_SIZE,
} mm_conf_entry_t;

typedef enum {
    MM_CONF_TYPE_I8,
    MM_CONF_TYPE_U8,
    MM_CONF_TYPE_I16,
    MM_CONF_TYPE_U16,
    MM_CONF_TYPE_I32,
    MM_CONF_TYPE_U32,
    MM_CONF_TYPE_STR,
} mm_conf_type_t;

void mm_conf_init(void);
int mm_conf_initialized(void);

mm_err_t mm_conf_set(mm_conf_entry_t entry, const void *value);
mm_err_t mm_conf_get(mm_conf_entry_t entry, void *value, size_t *size);
void *mm_conf_get_with_malloc(mm_conf_entry_t entry);
uint32_t mm_conf_get_as_integer(mm_conf_entry_t entry);
mm_err_t mm_conf_get_as_string(mm_conf_entry_t entry, char *buf, size_t *size);
mm_err_t mm_conf_set_from_string(mm_conf_entry_t entry, const char *buf);
mm_err_t mm_conf_restore_default(mm_conf_entry_t entry);
mm_err_t mm_conf_restore_all_defaults(void);

mm_conf_entry_t mm_conf_entry_by_name(const char *name);

int mm_conf_requires_reboot(mm_conf_entry_t entry);
mm_conf_type_t mm_conf_get_type(mm_conf_entry_t entry);
const char *mm_conf_get_name(mm_conf_entry_t entry);

#ifdef __cplusplus
}
#endif

#endif /* __MM_CONF_H */
