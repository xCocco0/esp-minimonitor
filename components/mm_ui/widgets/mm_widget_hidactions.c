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
#include "mm_hid_scheduler.h"
#include "mm_port_ui_customization.h"
#include "mm_widgets.h"
#include "mm_style.h"
#include "mm_font.h"
#include "mm_ui_screen.h"

/* touching for less than this is considered a tap,
   touching for more than this is considered a tap only if no scrolling is detected */
#define SCROLL_THRESHOLD_MS 150

#define __PRESS_START 0x1
#define __PRESS_END   0x2

/* We share 1 release timer for all controls */
static mm_widget_hidactions_control_t *s_current_touchpad = NULL;

lv_obj_t *mm_widget_hidactions_create(lv_obj_t *parent)
{
    lv_obj_t *c_obj;

    c_obj = lv_obj_class_create_obj(&mm_hidactions_class, parent);
    lv_obj_class_init_obj(c_obj);
    lv_obj_set_flex_flow(c_obj, LV_FLEX_FLOW_COLUMN);

    MM_STYLE_EMPTY_CONT(c_obj);

    return c_obj;
}

void mm_widget_hidactions_delete(lv_obj_t *widget)
{
    lv_obj_del(widget);
}

/* this function filters out ghost clicks (i.e. when the user scrolls the UI) */
static int __compute_press_actions(uint32_t *last_pressed, lv_event_code_t code)
{
    uint32_t delta;
    lv_indev_t * indev = lv_indev_get_act();
    int ret = 0;

    if(code == LV_EVENT_CLICKED) {
        return __PRESS_START | __PRESS_END;
    } else if(code == LV_EVENT_PRESSED) {
        *last_pressed = lv_tick_get();
    } else if(code == LV_EVENT_PRESSING) {

        if(*last_pressed) {
            delta = lv_tick_get() - *last_pressed;

            if(lv_indev_get_scroll_obj(indev)) {
                /* we started scrolling, skip button press until new PRESSED event */
            } else if(delta > SCROLL_THRESHOLD_MS) {
                /* we clicked for long enough without triggering a scroll */
                ret |= __PRESS_START;
                *last_pressed = 0;
            }
        }
    } else if(code == LV_EVENT_RELEASED) {
        if(*last_pressed) {
            delta = lv_tick_get() - *last_pressed;
            if(delta < SCROLL_THRESHOLD_MS) {
                /* this is a short click */
                ret |= __PRESS_START;
                *last_pressed = 0;
            }
        }
        if(*last_pressed == 0) {
            /* was pressed before */
            ret |= __PRESS_END;
        } else {
            /* we never really pressed the button */
            *last_pressed = 0;
        }
    }

    return ret;
}

static void __open_touchpad(lv_event_t *e)
{
    s_current_touchpad = (mm_widget_hidactions_control_t *) lv_event_get_user_data(e);

    MM_LOG_I("Opening touchpad");
    mm_ui_screen_switch(MM_SCREEN_TOUCHPAD);
}

/*                       START                       STOP
    SINGLE : periodic  : none                        none
             aperiodic : queue_macro                 none
    TOGGLE : periodic  : queue_periodic/cancel       none
             aperiodic : queue_down                  queue_up
    HOLD   : periodic  : queue_periodic              cancel
      MATRIX/aperiodic : queue_down                  queue_up
*/

static void __send_hid_scheduler_quick_press(uint32_t *id, const char *macro)
{
    *id = mm_hid_scheduler_queue_macro(macro);
}

static void __send_hid_scheduler_start_press(uint32_t *id, const char *macro, uint32_t period_ms, uint32_t jitter_ms)
{
    /* periodic: queue periodic event (key-down + key-up)
       else:     queue only key-down event */
    if(period_ms) {
        *id = mm_hid_scheduler_queue_periodic(macro, UINT32_MAX, period_ms, jitter_ms);
    } else {
        *id = mm_hid_scheduler_queue_down(macro);
    }
}

static void __send_hid_scheduler_stop_press(uint32_t *id, const char *macro, uint32_t period_ms, uint32_t jitter_ms)
{
    (void) jitter_ms;
    /* periodic: cancel periodic event + key-up
       else:     queue only key-up event */
    if(period_ms) {
        mm_hid_scheduler_cancel(*id);
    }
    *id = mm_hid_scheduler_queue_up(macro);
}

static void __button_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    mm_widget_hidactions_control_t *c = lv_event_get_user_data(e);
    int btn_index = 0;

    int press;
    mm_hid_press_behaviour_t press_bhv;
    char *macro = NULL;
    uint32_t period_ms = 0, jitter_ms = 0;

    if(c->conf->type == MM_HID_CONTROL_AUTOCLICK) {
        press_bhv = c->conf->u.autoclicker.press_bhv;
        macro = c->conf->u.autoclicker.macro;
        if(press_bhv != MM_HID_PRESS_ONCE) {
            period_ms = c->conf->u.autoclicker.period_ms;
            jitter_ms = c->conf->u.autoclicker.jitter_ms;
        }
    } else if(c->conf->type == MM_HID_CONTROL_MATRIX) {
        /* find which button triggered the event */
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
        /* note: MATRIX buttons use PRESS_HOLD */
        press_bhv = MM_HID_PRESS_HOLD;
        macro = c->conf->u.matrix.macro_v[btn_index];
    } else if(c->conf->type == MM_HID_CONTROL_TOUCHPAD) {
        press_bhv = MM_HID_PRESS_ONCE;
    } else {
        return;
    }

    press = __compute_press_actions(&c->last_pressed, code);

    switch(press_bhv) {
        case MM_HID_PRESS_ONCE:
            if(press & __PRESS_START) {
                if(c->conf->type == MM_HID_CONTROL_MATRIX) {
                    s_current_touchpad = (mm_widget_hidactions_control_t *) lv_event_get_user_data(e);
                    MM_LOG_I("Opening touchpad");
                    mm_ui_screen_switch(MM_SCREEN_TOUCHPAD);
                } else {
                    __send_hid_scheduler_quick_press(&c->cur_id, macro);
                }
            }
            if(press & __PRESS_END) {
                c->last_pressed = 0;
            }
            break;
        case MM_HID_PRESS_TOGGLE:
            if(press & __PRESS_START) {
                if(!c->key_down) {
                    __send_hid_scheduler_start_press(&c->cur_id, macro, period_ms, jitter_ms);
                    c->key_down = 1;
                } else {
                    __send_hid_scheduler_stop_press(&c->cur_id, macro, period_ms, jitter_ms);
                    c->key_down = 0;
                }
            }
            break;
        case MM_HID_PRESS_HOLD:
            if(press & __PRESS_START) {
                __send_hid_scheduler_start_press(&c->cur_id, macro, period_ms, jitter_ms);
            }
            if(press & __PRESS_END) {
                __send_hid_scheduler_stop_press(&c->cur_id, macro, period_ms, jitter_ms);
                c->last_pressed = 0;
            }
            break;
        default:
            break;
    }
}

void mm_hidactions_control_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    mm_widget_hidactions_control_t * c = (mm_widget_hidactions_control_t *) obj;

    c->last_pressed = 0;
    c->cur_id = 0;

    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
}

void mm_hidactions_control_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    mm_widget_hidactions_control_t *c = (mm_widget_hidactions_control_t *) obj;
    mm_hid_press_behaviour_t press_bhv;
    char *macro = NULL;
    uint32_t period_ms = 0, jitter_ms = 0;
    LV_UNUSED(class_p);

    /* release buttons */
    if(c->conf->type == MM_HID_CONTROL_AUTOCLICK) {
        press_bhv = c->conf->u.autoclicker.press_bhv;
        macro = c->conf->u.autoclicker.macro;
        if(press_bhv == MM_HID_PRESS_TOGGLE || press_bhv == MM_HID_PRESS_HOLD) {
            period_ms = c->conf->u.autoclicker.period_ms;
            jitter_ms = c->conf->u.autoclicker.jitter_ms;
            __send_hid_scheduler_stop_press(&c->cur_id, macro, period_ms, jitter_ms);
        }
    } else if(c->conf->type == MM_HID_CONTROL_MATRIX) {
        /* find which button triggered the event */
        for(int i = 0; i < c->conf->u.matrix.n_buttons; ++i) {
            macro = c->conf->u.matrix.macro_v[i];
            __send_hid_scheduler_stop_press(&c->cur_id, macro, 0, 0);
        }
    }
}

struct mm_hid_control_config_s *mm_widget_hidaction_last_touchpad_config(void)
{
    return s_current_touchpad->conf;
}

static uint32_t __pow(uint32_t x, uint32_t n)
{
    uint32_t ret = 1;
    while(n--) {
        ret *= x;
    }
    return ret;
}

static const char *__fmt_period(uint32_t ms)
{
    static char buf[16];
    int shift = 3;
    ldiv_t split;
    uint32_t d = ms;

    /* increase to show more decimals, 10^n -> at most n
       decimals (values greater than 1000 have no effect) */
    #define __DGT 1000

    if(d < 1000) {
        /* print as-is */
        sprintf(buf, "%"PRIu32"ms", ms);
    } else {
        while(d >= __DGT && shift > 0) {
            /* just dry run */
            d /= 10;
            shift--;
        }

        if(shift != 3) {
            /* restore original value */
            d = ms;
            /* actual division, with the correct approximation */
            d = ROUND_DIV(d, __pow(10, 3 - shift));
        }

        while((d % 10) == 0 && shift > 0) {
            d /= 10;
            shift--;
        }

        if(shift > 0) {
            split = ldiv((long) d, (long) __pow(10, shift));
            sprintf(buf, "%ld.%0*lds", split.quot, shift, split.rem);
        } else {
            /* no decimals */
            sprintf(buf, "%"PRIu32"s", d);
        }
    }

    return (const char *) buf;
}

#define __BTN_LBL_BUFSIZE CONFIG_MM_HID_MAX_NKRO*4

lv_obj_t *mm_widget_hidactions_control_add(lv_obj_t *parent, struct mm_hid_control_config_s *conf)
{
    lv_obj_t *obj, *text_cont;
    lv_obj_t *c_obj;
    mm_widget_button_t *btn;
    uint32_t primary_color;
    mm_widget_hidactions_control_t *c;
    const mm_img_src_t *ico;
    int index;
    mm_key_t key;
    mm_hid_press_behaviour_t press;

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
        index = lv_obj_has_class(obj, &mm_hidactions_control_class) ? ((mm_widget_hidactions_control_t *) obj)->conf->index : UINT16_MAX;
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
    c->conf = mm_hid_control_config_deepcopy(conf);
    c->cur_id = 0;
    c->last_pressed = 0;

    if(conf->type == MM_HID_CONTROL_AUTOCLICK) {

        press = conf->u.autoclicker.press_bhv;

        lv_obj_set_style_layout(c_obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(c_obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(c_obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        ico = NULL;
        if(conf->u.autoclicker.icon) {
            if(strnlen(conf->u.autoclicker.icon, sizeof(conf->u.autoclicker.icon)) > 0) {
                ico = mm_img_src(conf->u.autoclicker.icon);
            }
        }
        if(!ico) {
            /* set default icon based on first key in the macro */
            key = mm_hid_control_find_key_prefix(conf->u.autoclicker.macro);
            if(MM_KEY_IS_MOUSE(key)) {
                ico = MM_IMG_SRC_CLICK;
            } else {
                ico = MM_IMG_SRC_TAP;
            }
        }

        if(press == MM_HID_PRESS_TOGGLE) {
            btn = mm_widget_button_create_checkable(c_obj, ico, primary_color, 0);
            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_CLICKED, (void *) c);
        } else {
            /* single click or hold */
            btn = mm_widget_button_create(c_obj, ico, primary_color);
            if(press == MM_HID_PRESS_HOLD) {
                lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_PRESSED, (void *) c);
                lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_PRESSING, (void *) c);
                lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_RELEASED, (void *) c);
            } else {
                lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_CLICKED, (void *) c);
            }
        }

        free(btn);

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
        lv_label_set_text_fmt(obj, "[%s]", conf->u.autoclicker.macro);

        obj = lv_label_create(text_cont);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, MM_FONT_VERY_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);

        /* set label depending on press behaviour */
        if(conf->u.autoclicker.period_ms == 0 || press == MM_HID_PRESS_ONCE) {
            lv_label_set_text_static(obj, conf->u.autoclicker.press_bhv == MM_HID_PRESS_ONCE ?
                    "Press to run" : conf->u.autoclicker.press_bhv == MM_HID_PRESS_TOGGLE ?
                    "Press to toggle" : "Hold to run"
            );
        } else {
            lv_label_set_text_fmt(obj, "%s (%s)", conf->u.autoclicker.press_bhv == MM_HID_PRESS_TOGGLE ?
                    "Toggle periodic" : "Hold periodic",
                    __fmt_period(conf->u.autoclicker.period_ms)
            );
        }

    } else if(conf->type == MM_HID_CONTROL_MATRIX) {
        lv_obj_set_style_layout(c_obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(c_obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(c_obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        for(int i = 0; i < conf->u.matrix.n_buttons; ++i) {
            if(conf->u.matrix.macro_v[i] == NULL) {
                continue;
            }

            ico = NULL;
            if(conf->u.matrix.icon_v[i]) {
                ico = mm_img_src(conf->u.matrix.icon_v[i]);
                if(ico) {
                    btn = mm_widget_button_create(c_obj, ico, primary_color);
                } else {
                    /* icon not found: reuse icon name as label  */
                    btn = mm_widget_button_create_with_text(c_obj, conf->u.matrix.icon_v[i], primary_color);
                }
            } else {
                /* generate a new name from key names */
                key = mm_hid_control_find_key_prefix(conf->u.matrix.macro_v[i]);
                btn = mm_widget_button_create_with_text(c_obj, key ? MM_KEY_TO_STR(key) : "", primary_color);
            }

            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_PRESSED, (void *) c);
            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_PRESSING, (void *) c);
            lv_obj_add_event_cb(btn->button, __button_cb, LV_EVENT_RELEASED, (void *) c);

            free(btn);
        }

    } else if(conf->type == MM_HID_CONTROL_TOUCHPAD) {
        lv_obj_set_style_layout(c_obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(c_obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(c_obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

        btn = mm_widget_button_create(c_obj, MM_IMG_SRC_TOUCHPAD, primary_color);
        lv_obj_add_event_cb(btn->button, __open_touchpad, LV_EVENT_CLICKED, (void *) c);
        free(btn);

        obj = lv_label_create(c_obj);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(obj, MM_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_grow(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text_static(obj, "Open Touchpad");

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
        child_index = lv_obj_has_class(child, &mm_hidactions_control_class) ? ((mm_widget_hidactions_control_t *) child)->conf->index : 0;

        if(index == child_index) {
            lv_obj_del(child);
            break;
        }
    }

    if(lv_obj_get_child_cnt(obj) == 0) {
        lv_obj_clear_flag(mm_widgets.lbl_no_hid, LV_OBJ_FLAG_HIDDEN);
    }
}
