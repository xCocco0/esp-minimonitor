/**
 * @file
 *
 * @brief Utilities for handling sensor data and meter configs
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_hid_control.h"

#include "mm_persist.h"

const char *mm_key2str_map[MM_KEY_ENUM_MAX] = {
    [ 0 ] = "",
    #define FOR_EACH_KEY(MACRONAME, value, fancy_name, short_name) [ MM_##MACRONAME ] = short_name ,
    #include "mm_keys.def"
    #undef FOR_EACH_KEY
};

const char *mm_key2longstr_map[MM_KEY_ENUM_MAX] = {
    [ 0 ] = "",
    #define FOR_EACH_KEY(MACRONAME, value, fancy_name, short_name) [ MM_##MACRONAME ] = fancy_name ,
    #include "mm_keys.def"
    #undef FOR_EACH_KEY
};

const uint8_t mm_key2value_map[MM_KEY_ENUM_MAX] = {
    #define FOR_EACH_KEY(MACRONAME, value, fancy_name, short_name) [ MM_##MACRONAME ] = value ,
    #include "mm_keys.def"
    #undef FOR_EACH_KEY
};

mm_err_t mm_hid_control_keycode_to_fullname(char *buf, size_t bufsize, mm_key_t *keys, size_t nkeys)
{
    int ret;
    int i = 0;
    int empty = 1;

    while(i < nkeys) {
        if(MM_KEY_IS_VALID(keys[i])) {
            ret = snprintf(buf, bufsize, "%s+", MM_KEY_TO_STR(keys[i]));
            if(!MM_CHECK_W(ret >= 0)) {
                return MM_FAIL;
            }

            empty = 0;
            bufsize -= ret;
            buf += ret;
        }
        ++i;
    }

    if(bufsize) {
        if(empty) {
            /* empty string */
            buf[0] = '\0';
        } else {
            /* strip trailing plus */
            buf[-1] = '\0';
        }
    }

    return MM_OK;
}

static inline mm_key_t __find_keyname_match(const char *begin, const char *end)
{
    for(size_t i = 0; i < MM_KEY_ENUM_MAX; ++i) {
        if(MM_KEY_TO_STR(i)) {
            if(strncmp(begin, MM_KEY_TO_STR(i), end - begin) == 0) {
                /* this matches keys that are longer than param */
                if(strlen(MM_KEY_TO_STR(i)) == end - begin) {
                    /* exact match */
                    return (mm_key_t) i;
                }
            }
        }
    }
    return MM_KEY_NONE;
}

int mm_hid_control_fullname_to_key_arr(mm_key_t *keys, size_t keys_bufsize, const char *str)
{
    mm_key_t k;
    const char *next = str;
    int i = 0;

    MM_CHECK_E(keys);
    MM_CHECK_E(str);

    while(1) {
        str = next;
        next = strchrnul(str, '+');
        /* e.g. KMem++KMem+. -> "KMem+" + "KMem+"
           e.g. KPad0+KPad00. -> "KPad0" + "KPad00" */
        if(*next != '\0') {
            while(*(next+1) == '+' || *(next+1) == '\0') ++next;
        }

        k = __find_keyname_match(str, next);

        if(k != MM_KEY_NONE) {
            keys[i++] = k;
            if(i >= keys_bufsize) {
                break;
            }
        }
        if(*next != '\0') {
            while(*next == '+') ++next;
        } else {
            break;
        }
    }

    return i;
}

mm_err_t mm_hid_control_key_to_action(struct mm_hid_action_s *act, mm_key_t key, int16_t intensity)
{
    return mm_hid_control_key_arr_to_action(act, act, act, &key, 1, intensity);
}

mm_err_t mm_hid_control_key_arr_to_action(struct mm_hid_action_s *act_m, struct mm_hid_action_s *act_kb, struct mm_hid_action_s *act_c, mm_key_t *keys, size_t n_keys, int16_t intensity)
{
    int i_kb = 0;
    mm_err_t ret = MM_OK;

    MM_CHECK_E(keys != NULL);
    MM_CHECK_E(act_m != NULL);
    MM_CHECK_E(act_kb != NULL);
    MM_CHECK_E(act_c != NULL);

    /* set default values
       notes:
         - keycode param is not required
         - type is only set if the action is relevant */
    act_kb->type = -1;
    act_kb->u.keyboard_report.modifiers = 0;
    act_c->type = -1;
    act_m->type = -1;
    act_m->u.mouse_report.buttons = 0;
    act_m->u.mouse_report.x = 0;
    act_m->u.mouse_report.y = 0;
    act_m->u.mouse_report.wheel = 0;
    act_m->u.mouse_report.pad = 0;
    act_m->u.mouse_report.is_absolute = 0;

    /* sort hid keys into the respective hid actions */
    for(int k = 0; k < n_keys; ++k) {
        if(keys[k] == 0) {
            continue;
        } else if(MM_KEY_IS_KEYBOARD(keys[k])) {
            act_kb->type = MM_HID_ACTION_KEYBOARD;
            act_kb->u.keyboard_report.keycode[i_kb++] = MM_KEY_TO_VALUE(keys[k]);
        } else if(MM_KEY_IS_MODIFIER(keys[k])) {
            act_kb->type = MM_HID_ACTION_KEYBOARD;
            act_kb->u.keyboard_report.modifiers |= MM_KEY_TO_VALUE(keys[k]);
        } else if(MM_KEY_IS_CONSUMER(keys[k])) {
            act_c->type = MM_HID_ACTION_CONSUMER;
            act_c->u.consumer_report.control = MM_KEY_TO_VALUE(keys[k]);
        } else if(MM_KEY_IS_MOUSE(keys[k])) {
            act_m->type = MM_HID_ACTION_MOUSE;
            switch(keys[k]) {
                case MM_KEY_MOUSE_ABS_X:
                    act_m->u.mouse_report.x = intensity;
                    act_m->u.mouse_report.is_absolute = 1;
                    break;
                case MM_KEY_MOUSE_ABS_Y:
                    act_m->u.mouse_report.y = intensity;
                    act_m->u.mouse_report.is_absolute = 1;
                    break;
                case MM_KEY_MOUSE_DELTA_X:
                    act_m->u.mouse_report.x = intensity;
                    break;
                case MM_KEY_MOUSE_DELTA_Y:
                    act_m->u.mouse_report.y = intensity;
                    break;
                case MM_KEY_MOUSE_WHEEL:
                    act_m->u.mouse_report.wheel = intensity;
                    break;
                case MM_KEY_MOUSE_PAD:
                    act_m->u.mouse_report.pad = intensity;
                    break;
                default:
                    /* this is a mouse button */
                    act_m->u.mouse_report.buttons |= MM_KEY_TO_VALUE(keys[k]);
                    break;
            }
        } else {
            MM_LOG_W("Unexpected key id %d", keys[k]);
            ret = MM_FAIL;
            break;
        }
    }

    /* fill the rest with 0s */
    while(i_kb < 6) {
        act_kb->u.keyboard_report.keycode[i_kb++] = 0;
    }

    return ret;
}

mm_err_t mm_hid_control_config_to_action(struct mm_hid_action_s *act_m, struct mm_hid_action_s *act_kb, struct mm_hid_action_s *act_c, struct mm_hid_control_config_s *conf)
{
    mm_key_t *kv = NULL;
    int kv_len = 0;
    mm_err_t ret = MM_OK;
    int16_t intensity = 0;

    MM_CHECK_E(conf != NULL);

    /* get pointer to the keys array */
    switch(conf->type) {
        case MM_HID_CONTROL_AUTOCLICK:
            kv = &conf->u.autoclicker.keys[0];
            kv_len = CONFIG_MM_HID_MAX_NKRO;
            intensity = conf->u.autoclicker.intensity;
            break;
        default:
            MM_LOG_W("Trying to decode unsupported config type %d", conf->type);
            /* nothing to do */
            return MM_FAIL;
    }

    ret = mm_hid_control_key_arr_to_action(act_m, act_kb, act_c, kv, kv_len, intensity);

    return ret;
}

mm_err_t mm_hid_control_store_config(struct mm_hid_control_config_s *config)
{
    char key[32];
    /* store struct */
    sprintf(key, "hid%d", config->index);
    return mm_persist_store_obj(key, config, sizeof(struct mm_hid_control_config_s));
}

mm_err_t mm_hid_control_fetch_config(struct mm_hid_control_config_s *config)
{
    char key[32];
    size_t size;

    /* store struct */
    sprintf(key, "hid%d", config->index);
    size = sizeof(struct mm_hid_control_config_s);
    if(mm_persist_fetch_obj(key, config, &size) == MM_FAIL) {
        return MM_FAIL;
    }

    return MM_OK;
}

mm_err_t mm_hid_control_erase_config(int index)
{
    char key[32];
    sprintf(key, "hid%d", index);
    return mm_persist_invalidate_obj(key);
}
