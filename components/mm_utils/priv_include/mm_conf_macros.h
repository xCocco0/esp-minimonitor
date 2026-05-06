/**
 * @file
 *
 * @brief Utility macros for the config module
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_CONF_MACROS_H
#define __MM_CONF_MACROS_H

#define CONFIG_DEF_ENTRY(name, key, type, default_value, req_reb) __CONFIG_DEF_ENTRY_##type(name, key, default_value, req_reb)

#define __CONFIG_DEF_ENTRY_common(name_str, key_str, default_v, type_enum, union_member_name, req_reb) \
[ MM_CONF_##name_str ] = { \
    .name = #name_str, \
    .key = key_str, \
    .type = type_enum, \
    .default_value. union_member_name = default_v, \
    .requires_reboot = req_reb, \
}

#define __CONFIG_DEF_ENTRY_str(name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_STR, str, req_reb)
#define __CONFIG_DEF_ENTRY_i8( name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_I8,  i8,  req_reb)
#define __CONFIG_DEF_ENTRY_i16(name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_I16, i16, req_reb)
#define __CONFIG_DEF_ENTRY_i32(name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_I32, i32, req_reb)
#define __CONFIG_DEF_ENTRY_u8( name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_U8,  u8,  req_reb)
#define __CONFIG_DEF_ENTRY_u16(name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_U16, u16, req_reb)
#define __CONFIG_DEF_ENTRY_u32(name, key, default_v, req_reb) __CONFIG_DEF_ENTRY_common(name, key, default_v, MM_CONF_TYPE_U32, u32, req_reb)

#endif /* __MM_CONF_MACROS_H */
