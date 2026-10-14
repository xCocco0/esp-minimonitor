/**
 * @file
 *
 * @brief Utilities for handling sensor data and meter configs
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_hid_control.h"

#include <stdio.h>
#include "mm_macros.h"
#include "mm_persist.h"

const char *mm_key2str_map[MM_KEY_ENUM_MAX] = {
    [ 0 ] = "",
    #define FOR_EACH_KEY(MACRONAME, value, fancy_name, short_name) [ MM_##MACRONAME ] = short_name ,
    #include "mm_keys.def"
    #undef FOR_EACH_KEY
};

const uint8_t mm_key2strlen_map[MM_KEY_ENUM_MAX] = {
    [ 0 ] = 0,
    #define FOR_EACH_KEY(MACRONAME, value, fancy_name, short_name) [ MM_##MACRONAME ] = sizeof(short_name) - 1 ,
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

static mm_key_t s_sorted_keys[MM_KEY_ENUM_MAX];
static size_t s_sorted_keys_len = 0;


static int __cmp_keys_db_entries(const void *a, const void *b) {
    return strcmp(
        MM_KEY_TO_STR(*(mm_key_t *) a),
        MM_KEY_TO_STR(*(mm_key_t *) b)
    );
}

static void __fill_key_db(void)
{
    s_sorted_keys_len = 0;

    MM_LOG_I("Populating sorted keys DB.");

    for(int i = 0; i < MM_KEY_ENUM_MAX; i++) {
        if(MM_KEY_IS_VALID(i)) {
            s_sorted_keys[s_sorted_keys_len]  = (mm_key_t) i;
            s_sorted_keys_len++;
        }
    }

    MM_CHECK_E(s_sorted_keys_len > 0);
    qsort(s_sorted_keys, s_sorted_keys_len, sizeof(mm_key_t), __cmp_keys_db_entries);
}

mm_key_t mm_hid_control_find_key_prefix(const char *key_name)
{
    size_t low, mid, high;
    size_t hint = MM_KEY_ENUM_MAX;
    mm_key_t entry;
    const char *entry_name;

    if(s_sorted_keys_len == 0) {
        __fill_key_db();
    }

    if(!key_name) {
        return MM_KEY_NONE;
    }

    /* binary search */
    low = 0;
    high = s_sorted_keys_len - 1;
    while(low <= high) {
        mid = low + (high - low) / 2;
        if(strcmp(MM_KEY_TO_STR(s_sorted_keys[mid]), key_name) <= 0 || mid == 0) {
            hint = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    /* note: due to the prefix nature, if key_name is longer than the key to match,
             hint is any key which is longer than the real to match. */

    /* try shorter keys until we find the longest match */
    while(hint < MM_KEY_ENUM_MAX) {
        entry = s_sorted_keys[hint];
        entry_name = MM_KEY_TO_STR(entry);

        if(entry_name[0] < key_name[0]) {
                /* this is shortest key string */
            break;
        }

        if(strncmp(key_name, entry_name, MM_KEY_TO_STR_LEN(entry)) == 0) {
            /* match */
            return entry;
        }

        hint--;
    }

    return MM_KEY_NONE;
}

/* persistency */

static const struct mm_persist_complex_field_s s_schema_config_autoclick_fields[2] = {
    MM_PERSIST_COMPLEX_FIELD_STR(struct mm_hid_control_config_s, u.autoclicker.icon),
    MM_PERSIST_COMPLEX_FIELD_STR(struct mm_hid_control_config_s, u.autoclicker.macro)
};
static const struct mm_persist_complex_field_s s_schema_config_matrix_fields[2] = {
    MM_PERSIST_COMPLEX_FIELD_2D_STR(struct mm_hid_control_config_s, u.matrix.icon_v, u.matrix.n_buttons),
    MM_PERSIST_COMPLEX_FIELD_2D_STR(struct mm_hid_control_config_s, u.matrix.macro_v, u.matrix.n_buttons)
};

static const struct mm_persist_complex_schema_s s_schema_config_autoclick = {
    .struct_size = sizeof(struct mm_hid_control_config_s),
    .n_fields = 2,
    .fields = s_schema_config_autoclick_fields

}, s_schema_config_matrix = {
    .struct_size = sizeof(struct mm_hid_control_config_s),
    .n_fields = 2,
    .fields = s_schema_config_matrix_fields

}, s_schema_config_touchpad = {
    .struct_size = sizeof(struct mm_hid_control_config_s),
    .n_fields = 0,
    .fields = NULL
};

mm_err_t mm_hid_control_config_update(struct mm_hid_control_config_s *config)
{
    mm_err_t ret;
    char key[32];

    sprintf(key, "hid%d", config->index);
    switch(config->type) {
        case MM_HID_CONTROL_AUTOCLICK:
            ret = mm_persist_store_complex_obj(key, config, &s_schema_config_autoclick);
            break;
        case MM_HID_CONTROL_MATRIX:
            ret = mm_persist_store_complex_obj(key, config, &s_schema_config_matrix);
            break;
        default:
            ret = mm_persist_store_complex_obj(key, config, &s_schema_config_touchpad);
    }

    return ret;
}

struct mm_hid_control_config_s *mm_hid_control_config_fetch(int index)
{
    char key[32];
    void *obj;
    struct mm_hid_control_config_s *config;

    sprintf(key, "hid%d", index);

    /* we just peek at the base struct to check the type */
    config = mm_persist_fetch_obj_malloc(key, NULL);
    if(!config) {
        /* not found */
        return NULL;
    }

    switch(config->type) {
        case MM_HID_CONTROL_AUTOCLICK:
            obj = mm_persist_fetch_complex_obj_malloc(key, &s_schema_config_autoclick);
            break;
        case MM_HID_CONTROL_MATRIX:
            obj = mm_persist_fetch_complex_obj_malloc(key, &s_schema_config_matrix);
            break;
        default:
            obj = mm_persist_fetch_complex_obj_malloc(key, &s_schema_config_touchpad);
    }

    free(config);
    if(!obj) {
        /* not found */
        return NULL;
    }

    return (struct mm_hid_control_config_s *) obj;
}

void mm_hid_control_config_destroy(struct mm_hid_control_config_s *config)
{
    if(config) {
        free(config);
    }
}

mm_err_t mm_hid_control_config_erase(int index)
{
    char key[32];

    sprintf(key, "hid%d", index);
    return mm_persist_invalidate_complex_obj(key);
}

struct mm_hid_control_config_s *mm_hid_control_config_deepcopy(struct mm_hid_control_config_s *config)
{
    struct mm_hid_control_config_s *data;
    MM_CHECK_E(config != NULL);

    data = (struct mm_hid_control_config_s *) malloc(sizeof(struct mm_hid_control_config_s));
    MM_CHECK_MALLOC_E(data);
    memcpy(data, config, sizeof(struct mm_hid_control_config_s));

    if(config->type == MM_HID_CONTROL_AUTOCLICK) {
        if(config->u.autoclicker.macro) {
            data->u.autoclicker.macro = strdup(config->u.autoclicker.macro);
            MM_CHECK_MALLOC_E(data->u.autoclicker.macro);
        } else {
            data->u.autoclicker.macro = NULL;
        }
        if(config->u.autoclicker.icon) {
            data->u.autoclicker.icon = strdup(config->u.autoclicker.icon);
            MM_CHECK_MALLOC_E(data->u.autoclicker.icon);
        } else {
            data->u.autoclicker.icon = NULL;
        }
    } else if(config->type == MM_HID_CONTROL_MATRIX) {
        if(config->u.matrix.macro_v) {

            data->u.matrix.macro_v = calloc(config->u.matrix.n_buttons, sizeof(char *));
            MM_CHECK_MALLOC_E(data->u.matrix.macro_v);

            for(int i = 0; i < config->u.matrix.n_buttons; ++i) {
                if(config->u.matrix.macro_v[i]) {
                    data->u.matrix.macro_v[i] = strdup(config->u.matrix.macro_v[i]);
                    MM_CHECK_MALLOC_E(data->u.matrix.macro_v[i]);
                } else {
                    data->u.matrix.macro_v[i] = NULL;
                }
            }
        } else {
            data->u.matrix.macro_v = NULL;
        }
        if(config->u.matrix.icon_v) {

            data->u.matrix.icon_v = calloc(config->u.matrix.n_buttons, sizeof(char *));
            MM_CHECK_MALLOC_E(data->u.matrix.icon_v);

            for(int i = 0; i < config->u.matrix.n_buttons; ++i) {
                if(config->u.matrix.icon_v[i]) {
                    data->u.matrix.icon_v[i] = strdup(config->u.matrix.icon_v[i]);
                    MM_CHECK_MALLOC_E(data->u.matrix.icon_v[i]);
                } else {
                    data->u.matrix.icon_v[i] = NULL;
                }
            }
        } else {
            data->u.matrix.icon_v = NULL;
        }
    }

    return data;
}

void mm_hid_control_config_deepfree(struct mm_hid_control_config_s *config)
{
    MM_CHECK_E(config != NULL);

    if(config->type == MM_HID_CONTROL_AUTOCLICK) {
        if(config->u.autoclicker.macro) {
            free(config->u.autoclicker.macro);
        }
        if(config->u.autoclicker.icon) {
            free(config->u.autoclicker.icon);
        }
    } else if(config->type == MM_HID_CONTROL_MATRIX) {
        if(config->u.matrix.macro_v) {
            for(int i = 0; i < config->u.matrix.n_buttons; ++i) {
                if(config->u.matrix.macro_v[i]) {
                    free(config->u.matrix.macro_v[i]);
                }
            }
            free(config->u.matrix.macro_v);
        }
        if(config->u.matrix.icon_v) {
            for(int i = 0; i < config->u.matrix.n_buttons; ++i) {
                if(config->u.matrix.icon_v[i]) {
                    free(config->u.matrix.icon_v[i]);
                }
            }
            free(config->u.matrix.icon_v);
        }
    }
    free(config);
}
