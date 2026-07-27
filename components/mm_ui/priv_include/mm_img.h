/**
 * @file
 *
 * @brief Handle conversion from mm_icon to LVGL format
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_IMG_H
#define __MM_IMG_H

#include <lvgl.h>
#include "mm_icon.h"

#ifdef __cplusplus
extern "C"
{
#endif

union mm_img_src_u {
    uint8_t magic[0];
    lv_img_dsc_t img_dsc[0];
    const char file_path[0];
};

typedef union mm_img_src_u mm_img_src_t;

#define FOR_EACH_ICON(id, n, w, r) extern lv_img_dsc_t mm_img_src_##n ;
#include "mm_icons.def"
#undef FOR_EACH_ICON

/* note: the compiler will optimize and inline this constants */
#define FOR_EACH_ICON(id, n, w, r) static const mm_img_src_t* const MM_IMG_SRC_##id = (mm_img_src_t *) &mm_img_src_##n;
#include "mm_icons.def"
#undef FOR_EACH_ICON

#define MM_IMG_SRC(id) ((mm_img_src_t *) &mm_img_src_##n)

const mm_img_src_t *mm_img_src_builtin(mm_icon_builtin_t icon);
const mm_img_src_t *mm_img_src_from_path(const char *path);
const mm_img_src_t *mm_img_src(const char* str);
const mm_img_src_t *mm_img_src_nofail(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* __MM_IMG_H */
