/**
 * @file
 *
 * @brief UI styles syntax sugar
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_macros.h"
#include "mm_conf.h"
#include "mm_font.h"
#include "mm_err.h"
#include "mm_style.h"
#include "mm_theme.h"

lv_style_t mm_style_label;
lv_style_t mm_style_panel;
lv_style_t mm_style_screen;

static int initialized = 0;
static int s_is_dark_theme = 0;

static uint32_t __color_add(uint32_t color, uint8_t offset)
{
        uint8_t c[3] = {
                (color >>  0) & 0xff,
                (color >>  8) & 0xff,
                (color >> 16) & 0xff,
        };

        int16_t acc = 3*offset;
        int div = 3;
        uint8_t amt;

        for(int pass = 0; pass < 3 && div > 0 && acc > 0; ++pass) {
                amt = CEIL_DIV(acc,div);
                for(int i = 0; i < 3; ++i) {
                        if(c[i] >= 0xff && pass > 0) {
                                // pass
                        } else if(amt >= 0xff - c[i]) {
                                div -= 1;
                                acc -= 0xff - c[i];
                                c[i] = 0xff;
                        } else {
                                acc -= amt;
                                c[i] += amt;
                        }
                }
        }

        return (c[2] << 16) | (c[1] << 8) | (c[0] << 0);
}

static uint32_t __color_sub(uint32_t color, uint8_t offset)
{
        uint8_t c[3] = {
                (color >>  0) & 0xff,
                (color >>  8) & 0xff,
                (color >> 16) & 0xff,
        };

        int16_t acc = 3*offset;
        int div = 3;
        uint8_t amt;

        for(int pass = 0; pass < 3 && div > 0 && acc > 0; ++pass) {
                amt = CEIL_DIV(acc,div);
                for(int i = 0; i < 3; ++i) {
                        if(c[i] <= 0 && pass > 0) {
                                // pass
                        } else if(c[i] <= amt) {
                                div -= 1;
                                acc -= c[i];
                                c[i] = 0x0;
                        } else {
                                acc -= amt;
                                c[i] -= amt;
                        }
                }
        }

        return (c[2] << 16) | (c[1] << 8) | (c[0] << 0);
}

uint32_t mm_style_color_shaded(uint32_t color)
{
    uint32_t brightness = 0;
    static const uint8_t variation_min = 0x04, variation_max = 0x20;

    brightness += (color >>  0) & 0xff;
    brightness += (color >>  8) & 0xff;
    brightness += (color >> 16) & 0xff;
    brightness /= 3;

    uint8_t offset = (variation_max * (brightness) + variation_min * (0x100 - brightness)) / 0x100;

    //printf("offset for color %#08"PRIx32" is %"PRIx8"\n", color, offset);

    if(s_is_dark_theme) {
        return __color_add(color, offset);
    } else {
        return __color_sub(color, offset);
    }

}

void mm_style_init(void)
{
    lv_style_t *st;
    uint32_t color_card, color_detail, color_text, color_background, color_primary, color_secondary;
    lv_theme_t *th;
    lv_disp_t *disp;

    color_card = mm_conf_get_as_integer(MM_CONF_COLOR_CARD);
    color_detail = mm_conf_get_as_integer(MM_CONF_COLOR_DETAIL);
    color_text = mm_conf_get_as_integer(MM_CONF_COLOR_TEXT);
    color_background = mm_conf_get_as_integer(MM_CONF_COLOR_BACKGROUND);
    color_primary = mm_conf_get_as_integer(MM_CONF_COLOR_PRIMARY);
    color_secondary = mm_conf_get_as_integer(MM_CONF_COLOR_SECONDARY);

    if(lv_color_brightness(lv_color_hex(color_background)) < 128) {
        s_is_dark_theme = 1;
    }

    disp = lv_disp_get_default();
    MM_CHECK_E(disp != NULL);
    th = mm_theme_init(disp, lv_color_hex(color_primary), lv_color_hex(color_secondary), 1, MM_FONT_NORMAL);
    lv_disp_set_theme(disp, th);

    st = &mm_style_label;
    lv_style_init(st);

    lv_style_set_text_color(st, lv_color_hex(color_text));

    st = &mm_style_panel;
    lv_style_init(st);

    lv_style_set_bg_color(st, lv_color_hex(color_card));
    color_detail = mm_style_color_shaded(color_card);
    lv_style_set_border_color(st, lv_color_hex(color_detail));

    lv_style_set_border_opa(st, 255);
    lv_style_set_border_width(st, 2);
    lv_style_set_radius(st, 7);

    st = &mm_style_screen;
    lv_style_init(st);

    lv_style_set_bg_color(st, lv_color_hex(color_background));

    initialized = 1;
}

void mm_add_style_label(lv_obj_t *obj, lv_style_selector_t selector)
{

    if(MM_CHECK_W(initialized)) {
        //lv_obj_add_style(obj, &mm_style_label, selector);
    }
}
void mm_add_style_panel(lv_obj_t *obj, lv_style_selector_t selector)
{
    if(MM_CHECK_W(initialized)) {
        //lv_obj_add_style(obj, &mm_style_panel, selector);
        lv_obj_add_style(obj, &mm_theme_styles()->card, selector);
    }
}
void mm_add_style_screen(lv_obj_t *obj, lv_style_selector_t selector)
{
    if(MM_CHECK_W(initialized)) {
        //lv_obj_add_style(obj, &mm_style_screen, selector);
    }
}

#define BOUND_SUM_U8(x, v) ( (((x) + (v)) & ~0xff) ? ((v) >= 0 ? 0xff : 0) : ((x) + (v)))

void mm_style_recolor_panel(lv_obj_t *obj, uint32_t color, lv_style_selector_t selector)
{
    lv_color_t color_main = lv_color_hex(color);
    lv_color_t color_border = lv_color_hex(mm_style_color_shaded(color));

    lv_obj_set_style_bg_color(obj, color_main, selector);
    lv_obj_set_style_border_color(obj, color_border, selector);
}

lv_color_t mm_style_color_text(void) {
    return lv_color_hex(mm_conf_get_as_integer(MM_CONF_COLOR_TEXT));
}
lv_color_t mm_style_color_background(void) {
    return lv_color_hex(mm_conf_get_as_integer(MM_CONF_COLOR_BACKGROUND));
}
lv_color_t mm_style_color_card(void) {
    return lv_color_hex(mm_conf_get_as_integer(MM_CONF_COLOR_CARD));
}
lv_color_t mm_style_color_detail(void) {
    return lv_color_hex(mm_conf_get_as_integer(MM_CONF_COLOR_DETAIL));
}
lv_color_t mm_style_color_primary(void) {
    return lv_color_hex(mm_conf_get_as_integer(MM_CONF_COLOR_PRIMARY));
}
lv_color_t mm_style_color_secondary(void) {
    return lv_color_hex(mm_conf_get_as_integer(MM_CONF_COLOR_SECONDARY));
}
