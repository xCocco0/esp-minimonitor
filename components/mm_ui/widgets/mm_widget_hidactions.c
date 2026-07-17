/**
 * @file
 *
 * @brief HID actions control widget
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_widget_hidactions.h"

#include "esp_random.h"
#include "mm_event.h"
#include "mm_conf.h"
#include "mm_macros.h"
#include "mm_port_ui_customization.h"
#include "mm_widgets.h"
#include "mm_style.h"
#include "mm_font.h"
#include "mm_ui_screen.h"

/* touching for less than this is considered a tap,
   touching for more than this is considered a tap only if no scrolling is detected */
#define SCROLL_THRESHOLD_MS 150

/* We share 1 release timer for all controls */
static lv_timer_t *s_release_timer = NULL;
static mm_widget_hidactions_control_t *s_current_touchpad = NULL;

static void __release_timer_cb(lv_timer_t *timer);
static void __autoclick_timer_cb(lv_timer_t *timer);

lv_obj_t *mm_widget_hidactions_create(lv_obj_t *parent)
{
    lv_obj_t *c_obj;

    c_obj = lv_obj_class_create_obj(&mm_hidactions_class, parent);
    lv_obj_class_init_obj(c_obj);
    lv_obj_set_flex_flow(c_obj, LV_FLEX_FLOW_COLUMN);

    MM_STYLE_EMPTY_CONT(c_obj);

    /* release timer is shared by all */
    if(s_release_timer == NULL) {
        s_release_timer = lv_timer_create(__release_timer_cb, CONFIG_MM_HID_DEFAULT_PRESS_DURATION_MS, NULL);
        lv_timer_set_repeat_count(s_release_timer, -1);
        lv_timer_pause(s_release_timer);
    }

    return c_obj;
}

void mm_widget_hidactions_delete(lv_obj_t *widget)
{
    lv_obj_del(widget);
}

static inline void __send_events(mm_widget_hidactions_control_t *c)
{
    if(c->act_kb.type == MM_HID_ACTION_KEYBOARD) {
        mm_event_queue_sendhid(&c->act_kb);
    }
    if(c->act_m.type == MM_HID_ACTION_MOUSE) {
        mm_event_queue_sendhid(&c->act_m);
    }
    if(c->act_c.type == MM_HID_ACTION_CONSUMER) {
        mm_event_queue_sendhid(&c->act_c);
    }
}
static void __button_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    mm_widget_hidactions_control_t *c = lv_event_get_user_data(e);
    lv_indev_t * indev = lv_indev_get_act();
    int btn_index = 0;
    mm_key_t *keyv;
    uint32_t delta;

    for(int i = 0; i < lv_obj_get_child_cnt(&c->panel); i++) {
        lv_obj_t * child = lv_obj_get_child(&c->panel, i);

        if (child != NULL && lv_obj_check_type(child, &lv_btn_class)) {
            if(child == btn) {
                break;
            } else {
                btn_index++;
            }
        }
    }

    if(c->conf.type == MM_HID_CONTROL_AUTOCLICK) {
        if(c->repeat_timer) {
            /* periodic click */
            if(c->pressed) {
                lv_timer_pause(c->repeat_timer);
                lv_timer_pause(s_release_timer);
                __release_timer_cb(NULL);
                c->pressed = 0;
            } else {
                c->repeat_timer->user_data = (void *) c;
                lv_timer_ready(c->repeat_timer);
                lv_timer_resume(c->repeat_timer);
                c->pressed = 1;
            }
        } else if(c->conf.u.autoclicker.duration_ms > 0) {
            /* single click */
            lv_timer_reset(s_release_timer);
            lv_timer_set_period(s_release_timer, c->conf.u.autoclicker.duration_ms);
            lv_timer_resume(s_release_timer);
            __send_events(c);
        } else {
            /* toggle click */
            if(c->pressed) {
                __release_timer_cb(NULL);
                c->pressed = 0;
            } else {
                __send_events(c);
                c->pressed = 1;
            }
        }
    } else if(c->conf.type == MM_HID_CONTROL_MATRIX) {

        keyv = &c->conf.u.matrix.keys[btn_index][0];

        if(code == LV_EVENT_PRESSED) {
            c->pressed = lv_tick_get();
        } else if(code == LV_EVENT_PRESSING) {

            if(c->pressed) {
                delta = lv_tick_get() - c->pressed;

                if(lv_indev_get_scroll_obj(indev)) {
                    /* we started scrolling, skip button press until new PRESSED event */
                } else if(delta > SCROLL_THRESHOLD_MS) {
                    /* we clicked for long enough without triggering a scroll */
                    mm_hid_control_key_arr_to_action(&c->act_m, &c->act_kb, &c->act_c, keyv, sizeof(c->conf.u.matrix.keys[0]), c->conf.u.matrix.intensity);
                    /* note: we reuse kb for both keyboard and mouse */
                    __send_events(c);
                    c->pressed = 0;
                }
            }
        } else if(code == LV_EVENT_RELEASED) {
            if(c->pressed) {
                delta = lv_tick_get() - c->pressed;
                if(delta < SCROLL_THRESHOLD_MS) {
                    /* this is a short click */
                    mm_hid_control_key_arr_to_action(&c->act_m, &c->act_kb, &c->act_c, keyv, sizeof(c->conf.u.matrix.keys[0]), c->conf.u.matrix.intensity);
                    __send_events(c);
                    c->pressed = 0;
                }
            }
            if(c->pressed == 0) {
                /* was pressed before */
                __release_timer_cb(NULL);
            } else {
                /* we never really pressed the button */
                c->pressed = 0;
            }
        }
    }
}

static void __open_touchpad(lv_event_t *e)
{
    s_current_touchpad = (mm_widget_hidactions_control_t *) lv_event_get_user_data(e);

    MM_LOG_I("Opening touchpad");
    mm_ui_screen_switch(MM_SCREEN_TOUCHPAD);
}

static void __release_timer_cb(lv_timer_t *timer)
{
    struct mm_hid_action_s release = {
        .type = MM_HID_ACTION_RELEASE,
    };

    mm_event_queue_sendhid(&release);
    if(timer) {
        lv_timer_pause(timer);
    }
}

static void __autoclick_timer_cb(lv_timer_t *timer)
{
    uint32_t rnd;
    int32_t variation = 0;
    int16_t signed_rnd;
    mm_widget_hidactions_control_t *c = (mm_widget_hidactions_control_t *) timer->user_data;

    MM_CHECK_W(c->conf.type == MM_HID_CONTROL_AUTOCLICK);
    MM_CHECK_E(s_release_timer != NULL);

    if(c->conf.u.autoclicker.jitter_ms > 0) {
        rnd = esp_random();

        #if 1 /* Triangular distribution */
        signed_rnd = (int16_t) (rnd & UINT8_MAX) - (int16_t) ((rnd >> 8) & UINT8_MAX);
        variation = (int32_t)((signed_rnd * c->conf.u.autoclicker.jitter_ms) / UINT8_MAX);
        #else /* Uniform distribution */
        (void ) signed_rnd;
        variation = (rnd % (2 * c->conf.u.autoclicker.jitter_ms)) - c->conf.u.autoclicker.jitter_ms;
        #endif

        lv_timer_set_period(timer, c->conf.u.autoclicker.period_ms + variation);
    }

    lv_timer_reset(s_release_timer);
    lv_timer_set_period(s_release_timer, c->conf.u.autoclicker.duration_ms);
    lv_timer_resume(s_release_timer);

    __send_events(c);
}

void mm_hidactions_control_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    mm_widget_hidactions_control_t * c = (mm_widget_hidactions_control_t *) obj;

    c->repeat_timer = NULL;
    c->pressed = 0;

    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
}

void mm_hidactions_control_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    mm_widget_hidactions_control_t * c = (mm_widget_hidactions_control_t *)obj;
    LV_UNUSED(class_p);

    if(c->repeat_timer != NULL) {
        lv_timer_del(c->repeat_timer);
        c->repeat_timer = NULL;
    }

    lv_timer_pause(s_release_timer);
    __release_timer_cb(NULL);
}

struct mm_hid_control_config_s *mm_widget_hidaction_last_touchpad_config(void) {
    return &s_current_touchpad->conf;
}

#define __BTN_LBL_BUFSIZE CONFIG_MM_HID_MAX_NKRO*4

lv_obj_t *mm_widget_hidactions_control_add(lv_obj_t *parent, struct mm_hid_control_config_s *conf)
{
    lv_obj_t *obj, *text_cont;
    lv_obj_t *c_obj;
    mm_widget_button_t *btn;
    uint32_t primary_color;
    mm_widget_hidactions_control_t *c;
    mm_icon_t *ico;
    int index;
    char buf[__BTN_LBL_BUFSIZE];

    MM_CHECK_E(parent);
    MM_CHECK_E(conf);
    MM_CHECK_E(lv_obj_has_class(parent, &mm_hidactions_class));

    c_obj = lv_obj_class_create_obj(&mm_hidactions_control_class, parent);
    lv_obj_class_init_obj(c_obj);
    mm_add_style_panel(c_obj, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(c_obj, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_scrollbar_mode(c_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(c_obj, MM_PORT_FONT_SIZE_VERY_SMALL/2, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* reorder so that the sibling are sorted by index
       this is basically insertion sort */
    for(int i = 0; i < lv_obj_get_child_cnt(parent) - 1; ++i) {
        obj = lv_obj_get_child(parent, i);
        index = lv_obj_has_class(obj, &mm_hidactions_control_class) ? ((mm_widget_hidactions_control_t *) obj)->conf.index : UINT16_MAX;
        if(index > conf->index) {
            lv_obj_move_to_index(c_obj, i);
            break;
        }
    }

    /* hide the "No HID" label */
    lv_obj_add_flag(mm_widgets.lbl_no_hid, LV_OBJ_FLAG_HIDDEN);

    c = (mm_widget_hidactions_control_t *) c_obj;
    /* prepare control struct (timers are configured later) */
    primary_color = mm_conf_get_as_integer(MM_CONF_COLOR_PRIMARY);
    memcpy(&c->conf, conf, sizeof(struct mm_hid_control_config_s));
    c->repeat_timer = NULL;
    c->pressed = 0;

    if(conf->type == MM_HID_CONTROL_AUTOCLICK) {
        mm_hid_control_config_to_action(&c->act_m, &c->act_kb, &c->act_c, &c->conf);

        lv_obj_set_style_layout(c_obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(c_obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(c_obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        ico = NULL;
        if(strnlen(conf->u.autoclicker.icon, sizeof(conf->u.autoclicker.icon)) > 0) {
            ico = mm_icon_search_in_flash(conf->u.autoclicker.icon);
            if(!ico) {
                ico = mm_icon_search_in_sdcard(conf->u.autoclicker.icon);
            }
        }
        if(!ico) {
            /* default icon */
            if(c->act_m.type == MM_HID_ACTION_MOUSE) {
                ico = MM_ICON_CLICK;
            } else {
                ico = MM_ICON_TAP;
            }
        }

        if(conf->u.autoclicker.duration_ms > 0 && conf->u.autoclicker.period_ms == 0) {
            /* single click */
            btn = mm_widget_button_create(c_obj, ico, primary_color);
        } else {
            btn = mm_widget_button_create_checkable(c_obj, ico, primary_color, 0);
        }

        lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_CLICKED, (void *) c);

        free(btn);

        mm_hid_control_keycode_to_fullname(buf, sizeof(buf), conf->u.autoclicker.keys, CONFIG_MM_HID_MAX_NKRO);

        text_cont = lv_obj_create(c_obj);
        MM_STYLE_EMPTY_CONT(text_cont);
        lv_obj_set_size(text_cont, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_flex_grow(text_cont, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(text_cont, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(text_cont, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(text_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);

        obj = lv_label_create(text_cont);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text_fmt(obj, "[%s]", buf);

        obj = lv_label_create(text_cont);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);

        if(conf->u.autoclicker.duration_ms == 0) {
            /* toggle click */
            lv_label_set_text_static(obj, "Press to toggle");
            c->repeat_timer = NULL;
        } else {

            if(conf->u.autoclicker.period_ms == 0) {
                /* single click */
                lv_label_set_text_fmt(obj, "Press time: %"PRIu32"ms", conf->u.autoclicker.duration_ms);
                c->repeat_timer = NULL;
            } else {
                /* periodic click */
                lv_label_set_text_fmt(obj, "Autoclick: %"PRIu32"ms", conf->u.autoclicker.period_ms);

                c->repeat_timer = lv_timer_create(__autoclick_timer_cb, conf->u.autoclicker.period_ms, (void *) c);
                lv_timer_set_repeat_count(c->repeat_timer, -1);
                lv_timer_pause(c->repeat_timer);
                c->conf.u.autoclicker.duration_ms = MIN(conf->u.autoclicker.duration_ms, conf->u.autoclicker.period_ms);
            }
        }

    } else if(conf->type == MM_HID_CONTROL_MATRIX) {
        lv_obj_set_style_layout(c_obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(c_obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(c_obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        for(int i = 0; i < CONFIG_MM_HID_CONTROL_MATRIX_MAX_BTN; ++i) {
            if(conf->u.matrix.keys[i][0] == 0) {
                /* not a button */
                continue;
            }

            ico = NULL;
            if(strnlen(conf->u.matrix.icons[i], sizeof(conf->u.matrix.icons[0])) > 0) {
                ico = mm_icon_search_in_flash(conf->u.matrix.icons[i]);
                if(!ico) {
                    ico = mm_icon_search_in_sdcard(conf->u.matrix.icons[i]);
                }
                if(ico) {
                    btn = mm_widget_button_create(c_obj, ico, primary_color);
                } else {
                    /* icon not found: reuse icon name as label  */
                    btn = mm_widget_button_create_with_text(c_obj, conf->u.matrix.icons[i], primary_color);
                }
            } else {
                /* generate a new name from key names */
                mm_hid_control_keycode_to_fullname(buf, sizeof(buf), conf->u.matrix.keys[i], CONFIG_MM_HID_MAX_NKRO);
                btn = mm_widget_button_create_with_text(c_obj, buf, primary_color);
            }

            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_PRESSED, (void *) c);
            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_PRESSING, (void *) c);
            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_RELEASED, (void *) c);

            free(btn);
        }

        /* we use 1 timer for all buttons */
        c->repeat_timer = NULL;
        c->act_kb.type = -1;
        c->act_m.type = -1;
        c->act_c.type = -1;

    } else if(conf->type == MM_HID_CONTROL_TOUCHPAD) {
        lv_obj_set_style_layout(c_obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(c_obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(c_obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        btn = mm_widget_button_create(c_obj, MM_ICON_TOUCHPAD, primary_color);
        lv_obj_add_event_cb(btn->button, __open_touchpad, LV_EVENT_CLICKED, (void *) c);
        free(btn);

        obj = lv_label_create(c_obj);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_grow(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text_static(obj, "Open Touchpad");

        c->act_kb.type = -1;
        c->act_m.type = -1;
        c->act_c.type = -1;

    } else {
        MM_LOG_W("Wrong HID control type");
    }

    return c_obj;
}

void mm_widget_hidactions_control_delete(lv_obj_t *obj, int index)
{
    lv_obj_t *child;
    int child_index;

    for(int i = 0; i < lv_obj_get_child_cnt(obj); ++i) {
        child = lv_obj_get_child(obj, i);
        child_index = lv_obj_has_class(child, &mm_hidactions_control_class) ? ((mm_widget_hidactions_control_t *) child)->conf.index : 0;

        if(index == child_index) {
            lv_obj_del(child);
            break;
        }
    }

    if(lv_obj_get_child_cnt(obj) == 0) {
        lv_obj_clear_flag(mm_widgets.lbl_no_hid, LV_OBJ_FLAG_HIDDEN);
    }
}
