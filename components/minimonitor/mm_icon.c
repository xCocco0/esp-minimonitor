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

#include "mm_icon.h"

#include <stdio.h>
#include <sys/stat.h>
#include "sdkconfig.h"
#include "mm_log.h"
#include "mm_macros.h"

struct mm_builtin_icon_info_s {
    char *name;
    int width;
    int resizeable : 1;
};

static const struct mm_builtin_icon_info_s s_icon_info_map[MM_ICON_COUNT] = {
    #define FOR_EACH_ICON(id, n, w, r) [MM_ICON_##id] = {.name = STR(n), .width = w, .resizeable = r} ,
    #include "mm_icons.def"
    #undef FOR_EACH_ICON
};

mm_icon_builtin_t mm_icon_builtin(const char *name)
{
    mm_icon_builtin_t icon;
    for_each_icon(icon) {
        if(strcmp(name, s_icon_info_map[icon].name) == 0) {
            return icon;
        }
    }
    return MM_ICON_NONE;
}

const char *mm_icon_get_string_name(mm_icon_builtin_t icon)
{
    return s_icon_info_map[icon].name;
}

int mm_icon_get_size(mm_icon_builtin_t icon)
{
    return s_icon_info_map[icon].width;
}

int mm_icon_get_resizeable(mm_icon_builtin_t icon)
{
    return s_icon_info_map[icon].resizeable;
}
