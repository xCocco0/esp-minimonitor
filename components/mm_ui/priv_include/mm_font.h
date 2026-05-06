/**
 * @file
 *
 * @brief Handle fonts
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_FONT_H
#define __MM_FONT_H

#include <lvgl.h>
#include "mm_macros.h"
#include "mm_port_ui_customization.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* montserrat extensions */
extern const lv_font_t mm_font_montserrat_72;
extern const lv_font_t mm_font_montserrat_80;
extern const lv_font_t mm_font_montserrat_90;
extern const lv_font_t mm_font_montserrat_100;
extern const lv_font_t mm_font_montserrat_120;
extern const lv_font_t mm_font_montserrat_150;

#define MM_FONT_8  &lv_font_montserrat_8
#define MM_FONT_10 &lv_font_montserrat_10
#define MM_FONT_12 &lv_font_montserrat_12
#define MM_FONT_14 &lv_font_montserrat_14
#define MM_FONT_16 &lv_font_montserrat_16
#define MM_FONT_18 &lv_font_montserrat_18
#define MM_FONT_20 &lv_font_montserrat_20
#define MM_FONT_22 &lv_font_montserrat_22
#define MM_FONT_24 &lv_font_montserrat_24
#define MM_FONT_26 &lv_font_montserrat_26
#define MM_FONT_28 &lv_font_montserrat_28
#define MM_FONT_30 &lv_font_montserrat_30
#define MM_FONT_32 &lv_font_montserrat_32
#define MM_FONT_34 &lv_font_montserrat_34
#define MM_FONT_36 &lv_font_montserrat_36
#define MM_FONT_38 &lv_font_montserrat_38
#define MM_FONT_40 &lv_font_montserrat_40
#define MM_FONT_42 &lv_font_montserrat_42
#define MM_FONT_44 &lv_font_montserrat_44
#define MM_FONT_46 &lv_font_montserrat_46
#define MM_FONT_48 &lv_font_montserrat_48
#define MM_FONT_72 &mm_font_montserrat_72
#define MM_FONT_80 &mm_font_montserrat_80
#define MM_FONT_90 &mm_font_montserrat_90
#define MM_FONT_100 &mm_font_montserrat_100
#define MM_FONT_120 &mm_font_montserrat_120
#define MM_FONT_150 &mm_font_montserrat_150

#define __FROM_SIZE_TO_FONT(x) (__GLUE(MM_FONT_,x))

#define MM_FONT_TINY         __FROM_SIZE_TO_FONT(MM_PORT_FONT_SIZE_TINY)
#define MM_FONT_VERY_SMALL   __FROM_SIZE_TO_FONT(MM_PORT_FONT_SIZE_VERY_SMALL)
#define MM_FONT_SMALL        __FROM_SIZE_TO_FONT(MM_PORT_FONT_SIZE_SMALL)
#define MM_FONT_NORMAL       __FROM_SIZE_TO_FONT(MM_PORT_FONT_SIZE_NORMAL)
#define MM_FONT_LARGE        __FROM_SIZE_TO_FONT(MM_PORT_FONT_SIZE_LARGE)
#define MM_FONT_VERY_LARGE   __FROM_SIZE_TO_FONT(MM_PORT_FONT_SIZE_VERY_LARGE)

#ifdef __cplusplus
}
#endif

#endif /* __MM_FONT_H */
