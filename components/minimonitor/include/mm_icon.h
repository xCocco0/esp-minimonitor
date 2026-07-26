/**
 * @file
 *
 * @brief Icon utilities and declarations
 *
 * @note This module does not account for image data
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_ICON_H
#define __MM_ICON_H

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**< MM_ICON_xyz icon shorthands as enums */
enum mm_icon_builtin_e {
    MM_ICON_NONE = 0,
    #define FOR_EACH_ICON(id, name, width, resizeable) MM_ICON_##id ,
    #include "mm_icons.def"
    #undef FOR_EACH_ICON
    MM_ICON_COUNT,
};

typedef enum mm_icon_builtin_e mm_icon_builtin_t;

#define for_each_icon(icon) for(icon = (MM_ICON_NONE+1); icon < MM_ICON_COUNT; ++icon)

mm_icon_builtin_t mm_icon_builtin(const char *name);
const char *mm_icon_get_string_name(mm_icon_builtin_t icon);
int mm_icon_get_size(mm_icon_builtin_t icon);
int mm_icon_get_resizeable(mm_icon_builtin_t icon);

#ifdef __cplusplus
}
#endif

#endif /* __MM_ICON_H */
