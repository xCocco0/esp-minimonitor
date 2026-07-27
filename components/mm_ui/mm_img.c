/**
 * @file
 *
 * @brief Handle conversion from mm_icon to LVGL format
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_img.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <sys/stat.h>
#include "mm_icon.h"

static lv_img_dsc_t *s_dsc_map[MM_ICON_COUNT] = {
    #define FOR_EACH_ICON(id, n, w, r) [MM_ICON_##id] = &mm_img_src_##n ,
    #include "mm_icons.def"
    #undef FOR_EACH_ICON
};

static struct {
    char prefix[2];
    char path[62];
} s_buf = {
    .prefix = {LV_FS_STDIO_LETTER, ':'},
    .path = ""
};

const mm_img_src_t *mm_img_src_builtin(mm_icon_builtin_t icon)
{
    if(icon > MM_ICON_NONE && icon < MM_ICON_COUNT) {
        return (mm_img_src_t *) s_dsc_map[icon];
    } else {
        return NULL;
    }
}

const mm_img_src_t *mm_img_src_from_path(const char *path)
{
    /* path is either:
       - prefixed with /sdcard
       - prefixed with disk A:
       - postfixed with extension
       */
    #if CONFIG_MM_PORT_HAS_SDCARD
    struct stat st;

    const char *format[] = {
        ""CONFIG_MM_PORT_SDCARD_MOUNT_POINT"/%s.png",  /* foo */
        ""CONFIG_MM_PORT_SDCARD_MOUNT_POINT"/%s",      /* foo.png */
    };

    for(int i = 0; i < sizeof(format)/sizeof(format[0]); ++i) {
        snprintf(s_buf.path, sizeof(s_buf.path), format[i], path);

        if(stat(s_buf.path, &st) == 0) {
            return (mm_img_src_t *) &s_buf;
        }
    }

    #endif

    return NULL;
}

const mm_img_src_t *mm_img_src(const char* str)
{
    mm_icon_builtin_t icon = mm_icon_builtin(str);
    const mm_img_src_t *ret = NULL;

    if(icon != MM_ICON_NONE) {
        /* find match among builtin icons */
        ret = mm_img_src_builtin(icon);
    } else {
        /* find icon in sdcard */
        ret = mm_img_src_from_path(str);
    }

    return ret;
}

const mm_img_src_t *mm_img_src_nofail(const char* str)
{
    const mm_img_src_t *ret = mm_img_src(str);
    return ret ? ret : MM_IMG_SRC_BELL;
}
