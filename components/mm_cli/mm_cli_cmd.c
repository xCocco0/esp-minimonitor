/**
 * @file
 *
 * @brief CLI commands-related functions
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_cli_cmd.h"

#include <string.h>
#include "esp_app_desc.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_conf.h"
#include "mm_sensor.h"
#include "mm_macros.h"
#include "mm_icon.h"
#include "mm_event.h"
#include "esp_heap_caps.h"

static void __print_usage(FILE *f, const char *header, const char *prefix, void *argtable)
{
    if(header) {
        fprintf(f, "%s\n", header);
    }
    fprintf(f, "%s", prefix);
    arg_print_syntax(f, (void **) argtable, "\n");
    arg_print_glossary_gnu(f, (void **) argtable);
}

/* ================= hostname cmd ================= */
#if 1

static struct {
    struct arg_str *hostname;
    struct arg_end *end;
} __hostname_opt;

static int __hostname_cmd(int argc, char **argv);

static inline esp_err_t __register_hostname(void)
{
    esp_console_cmd_t hostname_cmd_config = {
        .command = "hostname",
        .help = "Set or read MDNS advertised hostname.",
        .hint = NULL,
        .func = &__hostname_cmd,
        .argtable = &__hostname_opt,
    };

    __hostname_opt.hostname = arg_strn(NULL, NULL, "<hostname>", 0, 1, "New hostname");
    __hostname_opt.end = arg_end(0);

    return esp_console_cmd_register(&hostname_cmd_config);
}

static int __hostname_cmd(int argc, char **argv)
{
    int ret = arg_parse(argc, argv, (void **) &__hostname_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __hostname_opt.end, argv[0]);
        return 1;
    }

    if(__hostname_opt.hostname->count == 1) {
        /* writing to config the new hostname */

        const char* hostname = __hostname_opt.hostname->sval[0];
        printf("Setting hostname to %s\n", hostname);
        mm_sethostname(hostname);

        /*TODO: mDNS*/

    } else {
        /* reading from config the current hostname */

        char *hostname_buf;
        size_t size;
        size = mm_gethostname(NULL, 0);
        hostname_buf = (char *) malloc(size);
        MM_CHECK_MALLOC_E(hostname_buf);
        mm_gethostname(hostname_buf, size);
        printf("hostname is %s\n", hostname_buf);
    }

    return 0;
}

#endif

/* ================= setsensor cmd ================= */
#if 1

static struct {
    /* note: an int is 32 bits on esp32 */
    struct arg_int *index;
    struct arg_int *value1;
    struct arg_int *value2;
    struct arg_int *value_panel;
    struct arg_end *end;
} __setsensor_opt;

static int __setsensor_cmd(int argc, char **argv);

/* cpu_load|cpu_temp|gpu_load|gpu_temp|fps|used_dram|total_dram */
static inline esp_err_t __register_setsensor(void)
{
    esp_console_cmd_t setsensor_cmd_config = {
        .command = "setsensor",
        .help = "Send sensor readings to display",
        .hint = NULL,
        .func = &__setsensor_cmd,
        .argtable = &__setsensor_opt,
    };

    __setsensor_opt.index =  arg_intn(NULL, NULL, "<index>", 1, 1, "Index of the meter to update");
    __setsensor_opt.value1 = arg_intn(NULL, NULL, "<value1>", 1, 1, "Value to write on the meter");
    __setsensor_opt.value2 = arg_intn(NULL, NULL, "<value2>", 0, 1, "Second argument to use in format string");
    __setsensor_opt.value_panel = arg_intn(NULL, NULL, "<value_panel>", 0, 1, "Value to write on the panel");
    __setsensor_opt.end = arg_end(0);

    return esp_console_cmd_register(&setsensor_cmd_config);
}

static int __setsensor_cmd(int argc, char **argv)
{
    int ret;
    uint32_t v1, v2 = 0, vP = 0;
    int index;

    ret = arg_parse(argc, argv, (void **) &__setsensor_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __setsensor_opt.end, argv[0]);
        return 1;
    }
    if(argc < 3) {
        __print_usage(stderr, "Usage:", argv[0], &__setsensor_opt);
        return 1;
    }

    index = __setsensor_opt.index->ival[0];
    v1 = __setsensor_opt.value1->ival[0];
    if(__setsensor_opt.value2->count > 0) v2 = __setsensor_opt.value2->ival[0];
    if(__setsensor_opt.value_panel->count > 0) vP = __setsensor_opt.value_panel->ival[0];

    mm_setsensor(index, v1, v2, vP);

    return 0;
}

#endif

/* ================= sensorconfig cmd ================= */
#if 1

static struct {
    struct arg_int *index;
    struct arg_str *label;
    struct arg_str *fmt;
    struct arg_int *max_value;
    struct arg_int *decimals;
    struct arg_int *timeout;
    struct arg_lit *flag_weak;
    struct arg_lit *flag_panel;
    struct arg_int *color_sensor;
    struct arg_int *wakeup_sensor;
    struct arg_int *panel_threshold;
    struct arg_str *panel_fmt;
    struct arg_int *color;
    struct arg_int *threshold;
    struct arg_int *wakeup_threshold;
    struct arg_end *end;
} __sensorconfig_opt;

static int __sensorconfig_cmd(int argc, char **argv);

static inline esp_err_t __register_sensorconfig(void)
{
    esp_console_cmd_t sensorconfig_cmd_config = {
        .command = "sensorconfig",
        .help = "Configure sensor meters.",
        .hint = NULL,
        .func = &__sensorconfig_cmd,
        .argtable = &__sensorconfig_opt,
    };

    __sensorconfig_opt.index =          arg_intn(NULL, NULL, "<index>", 1, 1, "Index of the meter");
    __sensorconfig_opt.label =          arg_strn(NULL, NULL, "<label>", 1, 1, "Label to display on the meter");
    __sensorconfig_opt.fmt =            arg_strn(NULL, NULL, "<format string>", 0, 1, "String format for side value");
    __sensorconfig_opt.max_value =      arg_intn("M", "max", NULL,      0, 1, "Maximum value for the meter bar");
    __sensorconfig_opt.timeout =        arg_intn("t", "timeout", NULL,  0, 1, "Timeout [ms]");
    __sensorconfig_opt.decimals =       arg_intn("d", "decimals", NULL, 0, 1, "Number of decimal digits");
    __sensorconfig_opt.flag_weak =      arg_litn("w", "weak",           0, 1, "Hide meter when no other is shown");
    __sensorconfig_opt.flag_panel =     arg_litn("p", "use_panel",      0, 1, "Show panel with meter");
    __sensorconfig_opt.wakeup_sensor =  arg_intn("W", "wakeup_sensor", NULL, 0, 1, "Which sensor to use to reset timeout");
    __sensorconfig_opt.color_sensor =   arg_intn("C", "color_sensor", NULL, 0, 1, "Which sensor to use to drive meter colors");
    __sensorconfig_opt.panel_fmt =      arg_strn("F","panel_fmt", "<format string>", 0, 1, "String format for panel value");
    __sensorconfig_opt.panel_threshold = arg_intn("S","panel_threshold", NULL, 0, 1, "Value threshold for showing the panel");
    __sensorconfig_opt.color =          arg_intn("c", "color", NULL,    0, 3, "Colors for the meter bar");
    __sensorconfig_opt.threshold =       arg_intn("s", "threshold", NULL, 0, 2, "Value threshold for switching color");
    __sensorconfig_opt.wakeup_threshold =arg_intn("a", "wakeup_threshold", NULL, 0, 1, "Value threshold for waking up the meter");
    __sensorconfig_opt.end =            arg_end(0);

    return esp_console_cmd_register(&sensorconfig_cmd_config);
}

static int __sensorconfig_cmd(int argc, char **argv)
{
    int ret;
    struct mm_sensor_meter_config_s param = MM_SENSOR_METER_CONFIG_DEFAULT;

    ret = arg_parse(argc, argv, (void **) &__sensorconfig_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __sensorconfig_opt.end, argv[0]);
        return 1;
    }

    if(__sensorconfig_opt.index->count == 0) {
        __print_usage(stderr, "Please specify an index:", argv[0], &__sensorconfig_opt);
        return 1;
    }

    if(__sensorconfig_opt.label->count == 0) {
        param.label[0] = '\0';
    } else {
        strncpy(param.label, __sensorconfig_opt.label->sval[0], sizeof(param.label));
    }

    param.index = __sensorconfig_opt.index->ival[0];
    if(__sensorconfig_opt.fmt->count > 0) strncpy(param.fmt, __sensorconfig_opt.fmt->sval[0], sizeof(param.fmt));
    if(__sensorconfig_opt.max_value->count > 0) param.max_value = __sensorconfig_opt.max_value->ival[0];
    if(__sensorconfig_opt.timeout->count > 0) param.timeout_ms = __sensorconfig_opt.timeout->ival[0];
    if(__sensorconfig_opt.color_sensor->count > 0) param.color_arg = __sensorconfig_opt.color_sensor->ival[0];
    if(__sensorconfig_opt.wakeup_sensor->count > 0) param.wakeup_arg = __sensorconfig_opt.wakeup_sensor->ival[0];
    if(__sensorconfig_opt.color->count > 0) param.color.base = __sensorconfig_opt.color->ival[0];
    if(__sensorconfig_opt.color->count > 1) param.color.warn = __sensorconfig_opt.color->ival[1];
    if(__sensorconfig_opt.color->count > 2) param.color.alert = __sensorconfig_opt.color->ival[2];
    if(__sensorconfig_opt.threshold->count > 0) param.wakeup_threshold = __sensorconfig_opt.threshold->ival[0];
    if(__sensorconfig_opt.threshold->count > 1) param.threshold.low = __sensorconfig_opt.threshold->ival[1];
    if(__sensorconfig_opt.threshold->count > 2) param.threshold.high = __sensorconfig_opt.threshold->ival[2];
    if(__sensorconfig_opt.flag_panel->count > 0) param.type = MM_METER_TYPE_PANEL;
    if(__sensorconfig_opt.flag_weak->count > 0) param.type = MM_METER_TYPE_WEAK;
    if(__sensorconfig_opt.panel_fmt->count > 0) strncpy(param.panel.fmt, __sensorconfig_opt.panel_fmt->sval[0], sizeof(param.panel.fmt));
    if(__sensorconfig_opt.panel_threshold->count > 0) param.panel.wakeup_threshold = __sensorconfig_opt.panel_threshold->ival[0];
    if(__sensorconfig_opt.wakeup_threshold->count > 0) param.wakeup_threshold = __sensorconfig_opt.wakeup_threshold->ival[0];
    if(__sensorconfig_opt.decimals->count > 0) param.decimals = __sensorconfig_opt.decimals->ival[0];

    mm_confsensor(&param);

    return 0;
}

#endif

/* ================= hidconfig cmd ================= */
#if CONFIG_MM_IO_ENABLE_USB_HID

static struct {
    struct arg_str *subcmd;
    struct arg_end *end;
} __hidconfig_opt;

static struct {
    struct arg_int *index;
    struct arg_str *keys;
    struct arg_str *icon;
    struct arg_lit *once;
    struct arg_lit *toggle;
    struct arg_int *period;
    struct arg_int *duration;
    struct arg_int *jitter;
    struct arg_int *intensity;
    struct arg_end *end;
} __hidconfig_autoclick_opt;

static struct {
    struct arg_int *index;
    struct arg_int *intensity;
    struct arg_str *keys;
    struct arg_str *icon;
    struct arg_end *end;
} __hidconfig_matrix_opt;

static struct {
    struct arg_int *index;
    struct arg_int *sens_x;
    struct arg_int *sens_y;
    struct arg_end *end;
} __hidconfig_touchpad_opt;

static int __hidconfig_cmd(int argc, char **argv);

static inline esp_err_t __register_hidconfig(void)
{
    esp_console_cmd_t hidconfig_cmd_config = {
        .command = "hidconfig",
        .help = "Configure HID controls.",
        .hint = NULL,
        .func = &__hidconfig_cmd,
        .argtable = &__hidconfig_opt,
    };

    __hidconfig_opt.subcmd = arg_strn(NULL, NULL, "<subcmd>", 1, 1, "Subcommand (autoclick|matrix|touchpad|remove|listkeys)");
    __hidconfig_opt.end    = arg_end(0);

    __hidconfig_autoclick_opt.index     = arg_intn(NULL, NULL, "<index>", 1, 1, "Index of the control");
    __hidconfig_autoclick_opt.keys      = arg_strn(NULL, NULL, "<keys>", 1, 1,"Key string");
    __hidconfig_autoclick_opt.icon      = arg_strn("i", "icon", "<name>", 0, 1, "Icon name");
    __hidconfig_autoclick_opt.once      = arg_litn("1", "once", 0, 1, "Single click mode");
    __hidconfig_autoclick_opt.toggle    = arg_litn("T", "toggle", 0, 1, "Toggle press/release mode");
    __hidconfig_autoclick_opt.period    = arg_intn("P", "period", "<ms>", 0, 1, "Autoclick period (ms)");
    __hidconfig_autoclick_opt.duration  = arg_intn("d", "duration","<ms>", 0, 1, "Press duration (ms)");
    __hidconfig_autoclick_opt.jitter    = arg_intn("j", "jitter", "<ms>", 0, 1, "Max period jitter (ms)");
    __hidconfig_autoclick_opt.intensity = arg_intn("n", "intensity","<val>", 0, 1, "Intensity");
    __hidconfig_autoclick_opt.end       = arg_end(0);

    __hidconfig_touchpad_opt.index  = arg_intn(NULL, NULL, "<index>", 1, 1, "Index of the control");
    __hidconfig_touchpad_opt.sens_x = arg_intn("x", NULL, "<val>", 0, 1, "X sensitivity");
    __hidconfig_touchpad_opt.sens_y = arg_intn("y", NULL, "<val>", 0, 1, "Y sensitivity");
    __hidconfig_touchpad_opt.end    = arg_end(0);

    __hidconfig_matrix_opt.index     = arg_intn(NULL, NULL, "<index>", 1, 1, "Index of the control");
    __hidconfig_matrix_opt.intensity = arg_intn("n", "intensity","<val>", 0, 1, "Intensity");
    __hidconfig_matrix_opt.keys      = arg_strn(NULL, NULL, "<keys ...>", 1, CONFIG_MM_HID_CONTROL_MATRIX_MAX_BTN, "Key string(s)");
    __hidconfig_matrix_opt.icon      = arg_strn("i", "icon", "<name>", 0, CONFIG_MM_HID_CONTROL_MATRIX_MAX_BTN, "Icon name(s)");
    __hidconfig_matrix_opt.end       = arg_end(0);

    return esp_console_cmd_register(&hidconfig_cmd_config);
}

static struct mm_hid_control_config_s confhid_param;

static inline int __hidconfig_autoclick_cmd(int argc, char **argv)
{
    int ret;

    if(argc < 3) {
        __print_usage(stderr, "Usage:", "hidconfig autoclick", &__hidconfig_autoclick_opt);
        return 1;
    }

    ret = arg_parse(argc, argv, (void **) &__hidconfig_autoclick_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __hidconfig_autoclick_opt.end, argv[0]);
        return 1;
    }

    confhid_param = MM_HID_CONTROL_CONFIG_DEFAULT_AUTOCLICK();

    if(__hidconfig_autoclick_opt.index->count > 0) {
        confhid_param.index = __hidconfig_autoclick_opt.index->ival[0];
    } else {
        __print_usage(stderr, "Usage:", "hidconfig autoclick", &__hidconfig_autoclick_opt);
        return 1;
    }

    ret = 0;
    if(__hidconfig_autoclick_opt.keys->count > 0) {
        ret = mm_hid_control_fullname_to_key_arr(confhid_param.u.autoclicker.keys, CONFIG_MM_HID_MAX_NKRO,
                __hidconfig_autoclick_opt.keys->sval[0]);
    }

    if(ret <= 0) {
        fprintf(stderr, "Could not parse key string\n");
        return 1;
    }

    /* icon name */
    if(__hidconfig_autoclick_opt.icon->count > 0) {
        strncpy(confhid_param.u.autoclicker.icon, __hidconfig_autoclick_opt.icon->sval[0], sizeof(confhid_param.u.autoclicker.icon));
    }
    /* duration */
    if(__hidconfig_autoclick_opt.duration->count > 0) {
        confhid_param.u.autoclicker.duration_ms = __hidconfig_autoclick_opt.duration->ival[0];
    }
    /* period */
    if(__hidconfig_autoclick_opt.period->count > 0) {
        confhid_param.u.autoclicker.period_ms = __hidconfig_autoclick_opt.period->ival[0];
    }
    /* intensity */
    if(__hidconfig_autoclick_opt.intensity->count > 0) {
        confhid_param.u.autoclicker.intensity = __hidconfig_autoclick_opt.intensity->ival[0];
    }
    /* jitter */
    if(__hidconfig_autoclick_opt.jitter->count > 0) {
        confhid_param.u.autoclicker.jitter_ms = __hidconfig_autoclick_opt.jitter->ival[0];
    }
    /* once */
    if(__hidconfig_autoclick_opt.toggle->count > 0) {
        confhid_param.u.autoclicker.duration_ms = 0;
    }
    /* toggle */
    if(__hidconfig_autoclick_opt.once->count > 0) {
        confhid_param.u.autoclicker.period_ms = 0;
        if(confhid_param.u.autoclicker.duration_ms == 0) {
            confhid_param.u.autoclicker.duration_ms = 1;
        }
    }

    mm_confhid(&confhid_param);

    return 0;
}

static inline int __hidconfig_matrix_cmd(int argc, char **argv)
{
    int ret;

    if(argc < 3) {
        __print_usage(stderr, "Usage:", "hidconfig matrix", &__hidconfig_matrix_opt);
        return 1;
    }

    ret = arg_parse(argc, argv, (void **) &__hidconfig_matrix_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __hidconfig_matrix_opt.end, argv[0]);
        return 1;
    }

    confhid_param = MM_HID_CONTROL_CONFIG_DEFAULT_MATRIX();

    if(__hidconfig_matrix_opt.index->count > 0) {
        confhid_param.index = __hidconfig_matrix_opt.index->ival[0];
    } else {
        __print_usage(stderr, "Usage:", "hidconfig matrix", &__hidconfig_matrix_opt);
        return 1;
    }

    if(__hidconfig_matrix_opt.intensity->count > 0) {
        confhid_param.u.matrix.intensity = __hidconfig_matrix_opt.intensity->ival[0];
    }

    for(int i = 0; i < CONFIG_MM_HID_CONTROL_MATRIX_MAX_BTN; ++i) {
        if(i < __hidconfig_matrix_opt.keys->count) {
            ret = mm_hid_control_fullname_to_key_arr(confhid_param.u.matrix.keys[i], CONFIG_MM_HID_MAX_NKRO,
                    __hidconfig_matrix_opt.keys->sval[i]);
            if(ret <= 0) {
                fprintf(stderr, "Could not parse key string\n");
                return 1;
            }
        }
        if(i < __hidconfig_matrix_opt.icon->count) {
            strcpy(&confhid_param.u.matrix.icons[i][0], __hidconfig_matrix_opt.icon->sval[i]);
        }
    }

    mm_confhid(&confhid_param);

    return 0;
}

static inline int __hidconfig_touchpad_cmd(int argc, char **argv)
{
    int ret;

    ret = arg_parse(argc, argv, (void **) &__hidconfig_touchpad_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __hidconfig_touchpad_opt.end, argv[0]);
        return 1;
    }

    memset(&confhid_param, 0, sizeof(struct mm_hid_control_config_s));

    confhid_param = MM_HID_CONTROL_CONFIG_DEFAULT_TOUCHPAD();

    if(__hidconfig_touchpad_opt.index->count > 0) {
        confhid_param.index = __hidconfig_touchpad_opt.index->ival[0];
    } else {
        __print_usage(stderr, "Usage:", "hidconfig touchpad", &__hidconfig_touchpad_opt);
        return 1;
    }

    if(__hidconfig_touchpad_opt.sens_x->count > 0) {
        confhid_param.u.touchpad.sensitivity_x = __hidconfig_touchpad_opt.sens_x->ival[0];
    }
    if(__hidconfig_touchpad_opt.sens_y->count > 0) {
        confhid_param.u.touchpad.sensitivity_y = __hidconfig_touchpad_opt.sens_y->ival[0];
    }

    mm_confhid(&confhid_param);

    return 0;
}

static inline int __hidconfig_remove_cmd(int argc, char **argv)
{
    int index;

    if(argc < 2) {
        fprintf(stderr, "Usage:\nhidconfig remove <index>\n");
        return 1;
    }

    index = atoi(argv[1]);

    confhid_param.index = index;
    confhid_param.type = MM_HID_CONTROL_INVALID;

    mm_confhid(&confhid_param);

    return 0;
}

static inline int __hidconfig_listkeys_cmd(int argc, char **argv)
{
    printf("%11c %11s %28s\n", ' ', "[Key Code]", "[Full Name]");
    for(int i = 1; i < MM_KEY_ENUM_MAX; ++i) {
        if(MM_KEY_IS_KEYBOARD(i)) {
            printf("%-11s %11s %28s\n", "[Keyboard]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
        if(MM_KEY_IS_MOUSE(i)) {
            printf("%-11s %11s %28s\n", "[Mouse]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
        if(MM_KEY_IS_MODIFIER(i)) {
            printf("%-11s %11s %28s\n", "[Modifier]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
        if(MM_KEY_IS_CONSUMER(i)) {
            printf("%-11s %11s %28s\n", "[Consumer]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
    }
    printf("\ne.g.: Shift+Ctrl+MouseLeft, MouseBack, KPad++KPad-\n");

    return 0;
}

static int __hidconfig_cmd(int argc, char **argv)
{
    int ret;

    if(argc < 2) {
        __print_usage(stderr, "Usage:", argv[0], &__hidconfig_opt);
        __print_usage(stderr, NULL, "hidconfig autoclick", &__hidconfig_autoclick_opt);
        __print_usage(stderr, NULL, "hidconfig matrix", &__hidconfig_matrix_opt);
        __print_usage(stderr, NULL, "hidconfig touchpad", &__hidconfig_touchpad_opt);
        return 1;
    }

    ret = arg_parse(2, argv, (void **) &__hidconfig_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __hidconfig_opt.end, argv[0]);
        return 1;
    }

    if(strcmp(__hidconfig_opt.subcmd->sval[0], "autoclick") == 0) {
        return __hidconfig_autoclick_cmd(argc-1, &argv[1]);

    } else if(strcmp(__hidconfig_opt.subcmd->sval[0], "matrix") == 0) {
        return __hidconfig_matrix_cmd(argc-1, &argv[1]);

    } else if(strcmp(__hidconfig_opt.subcmd->sval[0], "touchpad") == 0) {
        return __hidconfig_touchpad_cmd(argc-1, &argv[1]);

    } else if(strcmp(__hidconfig_opt.subcmd->sval[0], "remove") == 0) {
        return __hidconfig_remove_cmd(argc-1, &argv[1]);

    } else if(strcmp(__hidconfig_opt.subcmd->sval[0], "listkeys") == 0) {
        return __hidconfig_listkeys_cmd(argc-1, &argv[1]);
    }

    return 1;
}

#endif

/* ================= setwifi cmd ================= */
#if 1

static struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} __setwifi_opt;

static int __setwifi_cmd(int argc, char **argv);

static inline esp_err_t __register_setwifi(void)
{
    static esp_console_cmd_t setwifi_cmd_config = {
        .command = "setwifi",
        .help = "Set WiFi credentials.",
        .hint = NULL,
        .func = &__setwifi_cmd,
        .argtable = &__setwifi_opt,
    };

    __setwifi_opt.ssid = arg_strn("s", "ssid" ,"<ssid>", 0, 1, "WiFi station SSID (or empty string to suppress warnings)");
    __setwifi_opt.password = arg_strn("p", "password" ,"<password>", 0, 1, "WiFi station password");
    __setwifi_opt.end = arg_end(0);

    return esp_console_cmd_register(&setwifi_cmd_config);
}

static int __setwifi_cmd(int argc, char **argv)
{
    int ret;
    const char *ssid = NULL, *password = NULL;

    ret = arg_parse(argc, argv, (void **) &__setwifi_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __setwifi_opt.end, argv[0]);
        return 1;
    }
    if(argc <= 1) {
        __print_usage(stderr, "Usage:", argv[0], &__setwifi_opt);
        return 1;
    }

    if(__setwifi_opt.ssid->count > 0) {ssid = __setwifi_opt.ssid->sval[0];}
    if(__setwifi_opt.password->count > 0) {password = __setwifi_opt.password->sval[0];}

    mm_setwifi(ssid, password);

    printf("WiFi credentials updated.\n");

    return 0;
}

#endif

/* ================= reboot cmd ================= */
#if 1

static int __reboot_cmd(int argc, char **argv);

static inline esp_err_t __register_reboot(void)
{
    esp_console_cmd_t reboot_cmd_config = {
        .command = "reboot",
        .help = "Reboot device.",
        .hint = NULL,
        .func = &__reboot_cmd,
        .argtable = NULL,
    };

    return esp_console_cmd_register(&reboot_cmd_config);
}

static int __reboot_cmd(int argc, char **argv)
{
    int ret = 0;

    mm_reboot();

    return ret;
}

#endif

/* ================= date cmd ================= */
#if 1

static struct {
    struct arg_str *datestring;
    struct arg_end *end;
} __date_opt;

static int __date_cmd(int argc, char **argv);

static inline esp_err_t __register_date(void)
{
    esp_console_cmd_t date_cmd_config = {
        .command = "date",
        .help = "Set and show current time.",
        .hint = NULL,
        .func = &__date_cmd,
        .argtable = &__date_opt,
    };

    __date_opt.datestring = arg_strn("s", "set", "MMDDhhmm[[CC]YY][.ss]]]", 0, 1, "Set time according to string");
    __date_opt.end = arg_end(0);

    return esp_console_cmd_register(&date_cmd_config);
}

static int __date_cmd(int argc, char **argv)
{
    int ret;
    struct tm timeinfo = {0};
    char strftime_buf[64];
    struct timeval tv;


    ret = arg_parse(argc, argv, (void **) &__date_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __date_opt.end, argv[0]);
        return 1;
    }

    if(__date_opt.datestring->count > 0) {
        /* we are setting time */

        timeinfo.tm_year = 1970; /* this is the minimum year for mktime to work*/

        /* MMDDhhmm[[CC]YY][.ss] */
        ret = sscanf(__date_opt.datestring->sval[0], "%02u%02u%02u%02u%4u.%2u",
            &timeinfo.tm_mon,
            &timeinfo.tm_mday,
            &timeinfo.tm_hour,
            &timeinfo.tm_min,
            &timeinfo.tm_year,
            &timeinfo.tm_sec
        );

        switch(ret) {
            case 6:
                timeinfo.tm_sec =  timeinfo.tm_sec % 60;
                /* fall through */
            case 5:
                if(timeinfo.tm_year < 100) timeinfo.tm_year += 2000;
                timeinfo.tm_year = timeinfo.tm_year - 1900;
                /* fall through */
            case 4:
                timeinfo.tm_mon =  (timeinfo.tm_mon-1) % 12; /* 0-11*/
                timeinfo.tm_mday = timeinfo.tm_mday % 32; /* 1-31 */
                timeinfo.tm_hour = timeinfo.tm_hour % 60; /* 0-59 */
                timeinfo.tm_min =  timeinfo.tm_min  % 60; /* 0-59 */
                break;
            default:
                fprintf(stderr, "Could not parse date string.\n");
                return 1;
        }

        tv.tv_sec = mktime(&timeinfo);
        tv.tv_usec = 0;

        if(tv.tv_sec == -1) {
            return 1;
        }

        mm_settime(&tv);
    } else {
        /* we want to read time */
        mm_gettime(&timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("%s\n", strftime_buf);
    }

    return 0;
}

#endif

/* ================= sntpsync cmd ================= */
#if 1

static int __sntpsync_cmd(int argc, char **argv);

static inline esp_err_t __register_sntpsync(void)
{
    esp_console_cmd_t sntpsync_cmd_config = {
        .command = "sntpsync",
        .help = "Manually trigger an SNTP sync.",
        .hint = NULL,
        .func = &__sntpsync_cmd,
        .argtable = NULL,
    };

    return esp_console_cmd_register(&sntpsync_cmd_config);
}

static int __sntpsync_cmd(int argc, char **argv)
{
    int ret = 0;

    mm_sntptrigger();

    printf("SNTP time synchronization request sent.\n");

    return ret;
}

#endif

/* ================= notify cmd ================= */
#if 1

static struct {
    struct arg_str *icon_name;
    struct arg_str *notif_name;
    struct arg_int *duration;
    struct arg_int *color;
    struct arg_lit *flag_append;
    struct arg_lit *flag_flash; /* TODO: remove this argument */
    struct arg_lit *flag_replace;
    struct arg_str *msg;
    struct arg_end *end;
} __notify_opt;

static int __notify_cmd(int argc, char **argv);

static inline esp_err_t __register_notify(void)
{
    esp_console_cmd_t notify_cmd_config = {
        .command = "notify",
        .help = "Display notification on minimonitor.",
        .hint = NULL,
        .func = &__notify_cmd,
        .argtable = &__notify_opt,
    };

    __notify_opt.icon_name =    arg_strn("i", "icon", NULL, 0, 1, "Icon name");
    __notify_opt.notif_name =   arg_strn("n", "name", NULL, 0, 1, "Notification name");
    __notify_opt.msg =          arg_strn(NULL, NULL, "<message>", 0, 1, "Message to display");
    __notify_opt.duration =     arg_intn("d", "duration", NULL, 0, 1, "Notification duration [ms]");
    __notify_opt.color =        arg_intn("c", "color", "<hex>", 0, 1, "Notification theme color [RGB hex]" );
    __notify_opt.flag_append =  arg_litn("A", "append", 0, 1, "Always append a new notification");
    __notify_opt.flag_flash =   arg_litn("F", "flash", 0, 1, "Flash screen");
    __notify_opt.flag_replace = arg_litn("R", "replace", 0, 1, "Replace previous notification");
    __notify_opt.end = arg_end(0);

    return esp_console_cmd_register(&notify_cmd_config);
}

static int __notify_cmd(int argc, char **argv)
{
    int ret;

    ret = arg_parse(argc, argv, (void **) &__notify_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __notify_opt.end, argv[0]);
        return 1;
    }

    if(argc < 2) {
        __print_usage(stderr, "Usage:", argv[0], &__notify_opt);
        return 1;
    }

    const char *name = NULL;
    const char *icon_name = NULL;
    const char *msg = NULL;
    uint32_t color = 0x000000;
    int opt = MM_NOTIFY_DEFAULT;
    uint32_t duration_ms = CONFIG_MM_NOTIF_DEFAULT_DURATION;

    if(__notify_opt.flag_append->count  > 0) opt = MM_NOTIFY_APPEND;
    if(__notify_opt.flag_replace->count > 0) opt = MM_NOTIFY_REPLACE;
    if(__notify_opt.flag_flash->count   > 0) color = 0xffffff;
    if(__notify_opt.color->count > 0)        color = __notify_opt.color->ival[0];
    if(__notify_opt.duration->count > 0)     duration_ms = __notify_opt.duration->ival[0];
    if(__notify_opt.notif_name->count > 0)   name = __notify_opt.notif_name->sval[0];
    if(__notify_opt.icon_name->count > 0)    icon_name = __notify_opt.icon_name->sval[0];
    if(__notify_opt.msg->count > 0)          msg = __notify_opt.msg->sval[0];

    mm_notify(name, icon_name, color, duration_ms, msg, opt, 0);

    return 0;
}

#endif

/* ================= snooze cmd ================= */
#if 1

static struct {
    struct arg_str *notif_name;
    struct arg_end *end;
} __snooze_opt;

static int __snooze_cmd(int argc, char **argv);

static inline esp_err_t __register_snooze(void)
{
    esp_console_cmd_t snooze_cmd_config = {
        .command = "snooze",
        .help = "Snooze active notifications.",
        .hint = NULL,
        .func = &__snooze_cmd,
        .argtable = &__snooze_opt,
    };

    __snooze_opt.notif_name = arg_strn("n", "notification", NULL, 0, 1, "Name of the notification to snooze");
    __snooze_opt.end = arg_end(0);

    return esp_console_cmd_register(&snooze_cmd_config);
}

static int __snooze_cmd(int argc, char **argv)
{
    int ret;
    ret = arg_parse(argc, argv, (void **) &__snooze_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __snooze_opt.end, argv[0]);
        return 1;
    }

    if(__snooze_opt.notif_name->count > 0) {
        mm_snooze(__snooze_opt.notif_name->sval[0]);
    } else {
        mm_snooze(NULL);
    }

    return 0;
}

#endif

/* ================= tzset cmd ================= */
#if 1

static struct {
    struct arg_str *tz;
    struct arg_end *end;
} __tzset_opt;

static int __tzset_cmd(int argc, char **argv);

static inline esp_err_t __register_tzset(void)
{
    esp_console_cmd_t tzset_cmd_config = {
        .command = "tzset",
        .help = "Change timezone for time adjustment.",
        .hint = NULL,
        .func = &__tzset_cmd,
        .argtable = &__tzset_opt,
    };

    __tzset_opt.tz = arg_strn(NULL, NULL, "<tz>", 0, 1, "Timezone to use in TZ format");
    __tzset_opt.end = arg_end(0);

    return esp_console_cmd_register(&tzset_cmd_config);
}

static int __tzset_cmd(int argc, char **argv)
{
    int ret;
    ret = arg_parse(argc, argv, (void **) &__tzset_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __tzset_opt.end, argv[0]);
        return 1;
    }

    if(__tzset_opt.tz->count > 0) {
        /* setting a different timezone */
        mm_settimezone(__tzset_opt.tz->sval[0]);
    } else {
        /* reading current timezone */
        const char *tz = getenv("TZ");
        if(tz) {
            printf("TZ=%s\n", tz);
        }
    }

    return 0;
}

#endif

/* ================= config cmd ================= */
#if 1

static struct {
    struct arg_lit *all;
    struct arg_lit *restore;
    struct arg_str *key;
    struct arg_str *value;
    struct arg_end *end;
} __config_opt;

static int __config_cmd(int argc, char **argv);

static inline esp_err_t __register_config(void)
{
    esp_console_cmd_t config_cmd_config = {
        .command = "config",
        .help = "Modify minimonitor config entries.",
        .hint = NULL,
        .func = &__config_cmd,
        .argtable = &__config_opt,
    };

    __config_opt.all     = arg_litn("a", "all", 0, 1, "Print all config entries");
    __config_opt.restore = arg_litn("r", "restore-default", 0, 1, "Restore default value");
    __config_opt.key     = arg_strn(NULL, NULL, "<config_name>", 1, 1, "Config entry name");
    __config_opt.value   = arg_strn(NULL, NULL, "<new_value>", 0, 1, "Value to set");

    __config_opt.end = arg_end(0);

    return esp_console_cmd_register(&config_cmd_config);
}

static int __config_cmd_read_entry(mm_conf_entry_t entry)
{
    #define __VALUESTR_MAX_LEN 64
    size_t size = __VALUESTR_MAX_LEN;
    char buf[__VALUESTR_MAX_LEN];

    mm_conf_get_as_string(entry, buf, &size);
    printf("%s=%." STR(__VALUESTR_MAX_LEN) "s\n", mm_conf_get_name(entry), buf);

    return 0;
}

static int __config_cmd_write_entry(mm_conf_entry_t entry, const char* value)
{
    mm_conf_set_from_string(entry, value);

    if(mm_conf_requires_reboot(entry)) {
        printf("Config will be updated after reboot.\n");
    } else {
        printf("Config updated.\n");
    }

    return 0;
}

static int __config_cmd(int argc, char **argv)
{
    int ret;
    mm_conf_entry_t entry;

    ret = arg_parse(argc, argv, (void **) &__config_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __config_opt.end, argv[0]);
        return 1;
    }

    if(__config_opt.all->count > 0) {
        if(__config_opt.restore->count > 0) {
            /* The user asked to reset the whole config */
            ret = (mm_conf_restore_all_defaults() == MM_OK) ? 0 : 1;
        } else {
            /* Just print current config */
            for(int i = 0; i < MM_CONF_SIZE; ++i) {
                ret = __config_cmd_read_entry(i);
                if(ret != 0) {
                    return ret;
                }
            }
        }
    } else {

        if(__config_opt.key->count == 0) {
            __print_usage(stderr, "Usage:", argv[0], &__config_opt);
            return 1;
        }

        entry = mm_conf_entry_by_name(__config_opt.key->sval[0]);

        if(entry == MM_CONF_NONE) {
            fprintf(stderr, "Config entry '%s' does not exist.\n", __config_opt.key->sval[0]);
            return 1;
        }

        if(__config_opt.restore->count > 0) {
            ret = (mm_conf_restore_default(entry) == MM_OK) ? 0 : 1;
            if(mm_conf_requires_reboot(entry)) {
                printf("Config will be updated after reboot.\n");
            } else {
                printf("Config updated.\n");
            }
        } else if(__config_opt.value->count > 0) {
            ret = __config_cmd_write_entry(entry, __config_opt.value->sval[0]);
        } else {
            ret = __config_cmd_read_entry(entry);
        }
    }

    return ret;
}

#endif

/* ================= backlight cmd ================= */
#if 1

static struct {
    struct arg_int *bk;
    struct arg_end *end;
} __backlight_opt;

static int __backlight_cmd(int argc, char **argv);

static inline esp_err_t __register_backlight(void)
{
    esp_console_cmd_t backlight_cmd_config = {
        .command = "backlight",
        .help = "Adjust or show screen backlight.",
        .hint = NULL,
        .func = &__backlight_cmd,
        .argtable = &__backlight_opt,
    };

    __backlight_opt.bk = arg_intn(NULL, NULL, "<value>", 0, 1, "Backlight value (between 0 and 255)");
    __backlight_opt.end = arg_end(0);

    return esp_console_cmd_register(&backlight_cmd_config);
}

static int __backlight_cmd(int argc, char **argv)
{
    int ret;
    ret = arg_parse(argc, argv, (void **) &__backlight_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __backlight_opt.end, argv[0]);
        return 1;
    }

    if(__backlight_opt.bk->count > 0) {
        mm_setbacklight(__backlight_opt.bk->ival[0]);
        mm_conf_set(MM_CONF_BACKLIGHT, &__backlight_opt.bk->ival[0]);
    } else {
        uint8_t bk = (uint8_t) mm_conf_get_as_integer(MM_CONF_BACKLIGHT);
        printf("Backlight %"PRIu8"/255\n", bk);
    }

    return 0;
}

#endif

/* ================= log cmd ================= */
#if 1

static struct {
    struct arg_lit *l_none;
    struct arg_lit *l_error;
    struct arg_lit *l_warn;
    struct arg_lit *l_info;
    struct arg_lit *l_debug;
    struct arg_lit *l_verbose;
    struct arg_lit *l_all;
    struct arg_int *level;
    struct arg_str *module;
    struct arg_end *end;
} __log_opt;

static int __log_cmd(int argc, char **argv);

static inline esp_err_t __register_log(void)
{
    esp_console_cmd_t log_cmd_config = {
        .command = "log",
        .help = "Change log level. Note that only system-wide log level persists over reboots",
        .hint = NULL,
        .func = &__log_cmd,
        .argtable = &__log_opt,
    };

    __log_opt.level =      arg_intn("l", "level", NULL, 0, 1, "Log level (0-6)");
    __log_opt.l_none =     arg_litn("n", "none", 0, 1, "Log level: none");
    __log_opt.l_error =    arg_litn("e", "error", 0, 1, "Log level: error");
    __log_opt.l_warn =     arg_litn("w", "warn", 0, 1, "Log level: warn");
    __log_opt.l_info =     arg_litn("i", "info", 0, 1, "Log level: info");
    __log_opt.l_debug =    arg_litn("d", "debug", 0, 1, "Log level: debug");
    __log_opt.l_verbose =  arg_litn("v", "verbose", 0, 1, "Log level: verbose");
    __log_opt.l_all =      arg_litn("a", "all", 0, 1, "Log level: all");
    __log_opt.module =     arg_strn(NULL, NULL, "<module name>", 0, 1, "Module name (unspecified = all modules)");
    __log_opt.end =        arg_end(0);

    return esp_console_cmd_register(&log_cmd_config);
}

static int __log_cmd(int argc, char **argv)
{
    int ret;
    uint8_t log_level = UINT8_MAX;
    const char *module = "*";

    ret = arg_parse(argc, argv, (void **) &__log_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __log_opt.end, argv[0]);
        return 1;
    }

    if(__log_opt.l_none->count > 0)     {log_level = ESP_LOG_NONE;}
    if(__log_opt.l_error->count > 0)    {log_level = ESP_LOG_ERROR;}
    if(__log_opt.l_warn->count > 0)     {log_level = ESP_LOG_WARN;}
    if(__log_opt.l_info->count > 0)     {log_level = ESP_LOG_INFO;}
    if(__log_opt.l_debug->count > 0)    {log_level = ESP_LOG_DEBUG;}
    if(__log_opt.l_verbose->count > 0)  {log_level = ESP_LOG_VERBOSE;}
    if(__log_opt.l_all->count > 0)      {log_level = ESP_LOG_MAX;}
    if(__log_opt.level->count > 0)      {log_level = __log_opt.level->ival[0];}
    if(__log_opt.module->count > 0)     {module = __log_opt.module->sval[0];}

    if(log_level == UINT8_MAX) {
        __print_usage(stderr, "Usage:", argv[0], &__log_opt);
        return 1;
    }

    esp_log_level_set(module, log_level);

    /* store system-wide log level to nvs */
    if(module[0] == '*') {
        mm_conf_set(MM_CONF_LOGLEVEL, &log_level);
    }

    return 0;
}

#endif

/* ================= rotate cmd ================= */
#if 1

static struct {
    struct arg_str *mode;
    struct arg_end *end;
} __rotate_opt;

static int __rotate_cmd(int argc, char **argv);

static inline esp_err_t __register_rotate(void)
{
    esp_console_cmd_t rotate_cmd_config = {
        .command = "rotate",
        .help = "Fix screen rotation or enable automatic rotation",
        .hint = NULL,
        .func = &__rotate_cmd,
        .argtable = &__rotate_opt,
    };

    __rotate_opt.mode =    arg_strn(NULL, NULL, "<auto|degrees>", 0, 1, "Rotation mode. Write \"auto\" for automatic rotation, otherwise specify screen rotation in degrees.");
    __rotate_opt.end =     arg_end(0);

    return esp_console_cmd_register(&rotate_cmd_config);
}

static int __rotate_cmd(int argc, char **argv)
{
    int ret;
    uint8_t rot_deg = UINT8_MAX;

    ret = arg_parse(argc, argv, (void **) &__rotate_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __rotate_opt.end, argv[0]);
        return 1;
    }

    if(__rotate_opt.mode->count > 0) {
        const char * const mode = __rotate_opt.mode->sval[0];
        if(strcmp(mode, "portrait") == 0                  || strcmp(mode, "0") == 0) {
            rot_deg = 0;
        } else if(strcmp(mode, "landscape") == 0          || strcmp(mode, "1") == 0 || strcmp(mode, "90") == 0) {
            rot_deg = 1;
        } else if(strcmp(mode, "portrait_inverted") == 0  || strcmp(mode, "2") == 0 || strcmp(mode, "180") == 0) {
            rot_deg = 2;
        } else if(strcmp(mode, "landscape_inverted") == 0 || strcmp(mode, "3") == 0 || strcmp(mode, "270") == 0) {
            rot_deg = 3;
        } else if(strcmp(mode, "auto") == 0) {
            mm_conf_set(MM_CONF_AUTOROTATION, (uint8_t []) {1});
            return 0;
        }
    }

    if(rot_deg != UINT8_MAX) {
        mm_conf_set(MM_CONF_PREFERRED_ROTATION, &rot_deg);
        mm_conf_set(MM_CONF_AUTOROTATION, (uint8_t []) {0});
        /* TODO: update rotation immediately */
        printf("Preferred rotation set.\n");
    } else {
        __print_usage(stderr, "Usage:", argv[0], &__rotate_opt);
        return 1;
    }

    return 0;
}

#endif

/* ================= info cmd ================= */
#if 1

static struct {
    struct arg_str *cmd;
    struct arg_lit *ui;
    struct arg_end *end;
} __info_opt;

static int __info_cmd(int argc, char **argv);

static inline esp_err_t __register_info(void)
{
    esp_console_cmd_t info_cmd_config = {
        .command = "info",
        .help = "Get system information",
        .hint = NULL,
        .func = &__info_cmd,
        .argtable = &__info_opt,
    };

    __info_opt.cmd = arg_strn(NULL, NULL, "<subcmd>", 0, 1, "Subcommand (icons|keys|network|tasks).");
    __info_opt.ui =  arg_litn("u", "ui", 0, 1, "Show output on screen UI.");
    __info_opt.end = arg_end(0);

    return esp_console_cmd_register(&info_cmd_config);
}

static int __info_icons_cmd(int argc, char **argv)
{
    mm_icon_builtin_t icon;
    const char *icon_name;

    mm_snooze("icon_preview");
    if(__info_opt.ui->count > 0) {
        for_each_icon(icon) {
            icon_name = mm_icon_get_string_name(icon);
            mm_notify("icon_preview", icon_name, 0, 0, icon_name, MM_NOTIFY_APPEND, 0);
        }
    } else {
        printf("Available icons (builtin):\n");
        printf("%-20s %6s\n", "[Icon name]", "[Size]");
        for_each_icon(icon) {
            printf("%-20s %6d\n", mm_icon_get_string_name(icon), mm_icon_get_size(icon));
        }
    }
    return 0;
}

static int __info_keys_cmd(int argc, char **argv)
{
    #if CONFIG_MM_IO_ENABLE_USB_HID
    printf("%11c %11s %28s\n", ' ', "[Key Code]", "[Full Name]");
    for(int i = 1; i < MM_KEY_ENUM_MAX; ++i) {
        if(MM_KEY_IS_KEYBOARD(i)) {
            printf("%-11s %11s %28s\n", "[Keyboard]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
        if(MM_KEY_IS_MOUSE(i)) {
            printf("%-11s %11s %28s\n", "[Mouse]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
        if(MM_KEY_IS_MODIFIER(i)) {
            printf("%-11s %11s %28s\n", "[Modifier]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
        if(MM_KEY_IS_CONSUMER(i)) {
            printf("%-11s %11s %28s\n", "[Consumer]", MM_KEY_TO_STR(i), MM_KEY_TO_LONG_STR(i));
        }
    }
    printf("\ne.g.: Shift+Ctrl+MouseLeft, MouseBack, KPad++KPad-\n");
    #else
    printf("USB HID is disabled.");
    #endif
    return 0;
}

static const char *__rssi_qualitative(int8_t rssi)
{
    if(rssi > -30) {
        return "Too strong";
    } else if(rssi > -68) {
        return "Excellent";
    } else if(rssi > -71) {
        return "Strong";
    } else if(rssi > -80) {
        return "Weak";
    } else {
        return "Very weak";
    }
}

static int __info_network_cmd(int argc, char **argv)
{
    int ret;
    struct mm_wifi_info_s info;

    ret = mm_wifistatus(&info);

    if(ret == MM_YES) {
        printf("Status: Connected\n");
        printf("SSID:   %s [%02x:%02x:%02x:%02x:%02x:%02x]\n", info.ssid,
                info.bssid[0], info.bssid[1], info.bssid[2], info.bssid[3], info.bssid[4], info.bssid[5]);
        printf("IP:     %d.%d.%d.%d\n", info.ipv4[0], info.ipv4[1], info.ipv4[2], info.ipv4[3]);
        printf("RSSI:   %"PRId8" [%s]\n", info.rssi, __rssi_qualitative(info.rssi));
    } else {
        printf("Status: Not connected\n");
    }

    return 0;
}

#if CONFIG_MM_EVENT_TRACE_CPU_USAGE
static const char *__event_name(mm_event_t id)
{
    switch(id) {
        case MM_EVENT_PERIODIC:      return "periodic";
        case MM_EVENT_BACKGROUND:    return "background";
        case MM_EVENT_SETTIME:       return "settime";
        case MM_EVENT_NOTIFY:        return "notify";
        case MM_EVENT_SNOOZE:        return "snooze";
        case MM_EVENT_SETSENSOR:     return "setsensor";
        case MM_EVENT_CONFSENSOR:    return "confsensor";
        case MM_EVENT_REBOOT:        return "reboot";
        case MM_EVENT_SNTPTRIGGER:   return "sntptrigger";
        case MM_EVENT_SETHOSTNAME:   return "sethostname";
        case MM_EVENT_SETTIMEZONE:   return "settimezone";
        case MM_EVENT_SETWIFI:       return "setwifi";
        case MM_EVENT_CHANGETILE:    return "changetile";
        case MM_EVENT_SETBACKLIGHT:  return "setbacklight";
        case MM_EVENT_BSOD:          return "bsod";
        case MM_EVENT_STARTUP:       return "startup";
        case MM_EVENT_MOTION:        return "motion";
        case MM_EVENT_CONFHID:       return "confhid";
        case MM_EVENT_SENDHID:       return "sendhid";
        case MM_EVENT_DATA_REQUEST:  return "data_request";
        default:                     break;
    }

    MM_LOG_W("Unknown event id %d", id);
    return "";
}
#endif

static uint32_t __freertos_counter_to_ms(configRUN_TIME_COUNTER_TYPE count)
#if CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER
{
    /* count is in microseconds */
    return count/1000;
}
#else
{
    /* we don't know the units: measure a sample */
    static configRUN_TIME_COUNTER_TYPE sample_delta = 0;

    if(!sample_delta) {
        sample_delta = portGET_RUN_TIME_COUNTER_VALUE();
        /* wait 10 ticks.
           on average, this will block for 9.5*portTICK_PERIOD_MS milliseconds due to tick period precision */
        vTaskDelay(10);
        sample_delta = portGET_RUN_TIME_COUNTER_VALUE() - sample_delta;
        printf("sample_delta = %"PRIu32"\n", sample_delta);
    }
    return count * (9 * portTICK_PERIOD_MS + portTICK_PERIOD_MS/2) / sample_delta;
}
#endif

static int __info_tasks_cmd(int argc, char **argv)
{
    #if CONFIG_FREERTOS_USE_TRACE_FACILITY && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    UBaseType_t ntasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *status = calloc(ntasks, sizeof(TaskStatus_t));
    configRUN_TIME_COUNTER_TYPE runtime = 0;
    #endif
    #if CONFIG_MM_EVENT_TRACE_CPU_USAGE
    uint32_t event_cpu_time_ms;
    TickType_t uptime = 0;
    #endif
    int printed = 0;

    /* print freertos tasks info */

    #if CONFIG_FREERTOS_USE_TRACE_FACILITY && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS

    printed = 1;
    ntasks = uxTaskGetSystemState(status, ntasks, &runtime);

    printf("FreeRTOS tasks CPU time:\n");
    for(int i = 0; i < ntasks; ++i) {
        printf("%-20s %9"PRIu32" ms (%2d.%01d%%)\n",
                status[i].pcTaskName, __freertos_counter_to_ms(status[i].ulRunTimeCounter),
                (int) (100 * status[i].ulRunTimeCounter / runtime),
                (int) ((10 * 100 * status[i].ulRunTimeCounter / runtime) % 10));
    }
    #endif

    /* print mm_event handler info */

    #if CONFIG_MM_EVENT_TRACE_CPU_USAGE

    printed = 1;
    uptime = pdTICKS_TO_MS(xTaskGetTickCount());

    printf("Minimonitor events CPU time:\n");
    for(int i = 0; i < MM_EVENT_MAX; ++i) {
        event_cpu_time_ms = mm_event_cpu_usage(i);
        printf("%-20s %9"PRIu32" ms (%2d.%01d%%)\n",
                __event_name(i), event_cpu_time_ms,
                (int) (100 * event_cpu_time_ms / uptime),
                (int) ((10 * 100 * event_cpu_time_ms / uptime) % 10));
    }
    #endif

    if(!printed) {
        fprintf(stderr, "No way to monitor task usage.\n");
        (void) __freertos_counter_to_ms;
    }

    return 0;
}

static int __info_cmd(int argc, char **argv)
{
    int ret;
    multi_heap_info_t info;
    char *hostname;

    ret = arg_parse(argc, argv, (void **) &__info_opt);
    if(ret != 0) {
        arg_print_errors(stderr, __info_opt.end, argv[0]);
        return 1;
    }

    if(__info_opt.cmd->count > 0) {
        /* call subcommand */
        const char * const mode = __info_opt.cmd->sval[0];
        if(strcmp(mode, "icons") == 0) {
            ret = __info_icons_cmd(argc - 1, &argv[1]);
        } else if(strcmp(mode, "keys") == 0) {
            ret = __info_keys_cmd(argc - 1, &argv[1]);
        } else if(strcmp(mode, "network") == 0) {
            ret = __info_network_cmd(argc - 1, &argv[1]);
        } else if(strcmp(mode, "tasks") == 0) {
            ret = __info_tasks_cmd(argc - 1, &argv[1]);
        }

    } else {
        /* not subcommand: print generic infos */
        printf(MINIMONITOR_ASCII_ART_STR "\n");
        printf("\n");
        printf("MiniMonitor v%s\n", esp_app_get_description()->version);
        printf("\n");
        printf("Compiled: %s, %s\n", __DATE__, __TIME__);
        printf("Target board: %s\n", CONFIG_MM_PORT_TARGET_BOARD);
        printf("Target architecure: "
                #if CONFIG_IDF_TARGET_ARCH_XTENSA
                "Xtensa"
                #elif CONFIG_IDF_TARGET_ARCH_RISCV
                "RISC-V"
                #elif CONFIG_IDF_TARGET_LINUX
                "Linux"
                #else
                "Unknown"
                #endif
                "\n");
        heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
        printf("RAM usage: %dkB/%dkB\n", info.total_allocated_bytes / 1024, (info.total_allocated_bytes + info.total_free_bytes) / 1024);
        #if CONFIG_SPIRAM
        heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
        printf("SPIRAM usage %dkB/%dkB\n", info.total_allocated_bytes / 1024, (info.total_allocated_bytes + info.total_free_bytes) / 1024);
        #endif
        hostname = mm_conf_get_with_malloc(MM_CONF_HOSTNAME);
        if(hostname) {
            printf("Hostname: %s\n", hostname);
            free(hostname);
        }
        printf("Current time: ");
        __date_cmd(0, NULL);
        ret = 0;
    }

    return 0;
}

#endif

/*==================================*/

void mm_cli_cmd_register_all(void)
{
    MM_CHECK_E(__register_hostname() == ESP_OK);
    MM_CHECK_E(__register_setsensor() == ESP_OK);
    MM_CHECK_E(__register_sensorconfig() == ESP_OK);
    MM_CHECK_E(__register_setwifi() == ESP_OK);
    MM_CHECK_E(__register_reboot() == ESP_OK);
    MM_CHECK_E(__register_date() == ESP_OK);
    MM_CHECK_E(__register_sntpsync() == ESP_OK);
    MM_CHECK_E(__register_notify() == ESP_OK);
    MM_CHECK_E(__register_snooze() == ESP_OK);
    MM_CHECK_E(__register_tzset() == ESP_OK);
    MM_CHECK_E(__register_config() == ESP_OK);
    MM_CHECK_E(__register_backlight() == ESP_OK);
    MM_CHECK_E(__register_log() == ESP_OK);
    MM_CHECK_E(__register_rotate() == ESP_OK);
    #if CONFIG_MM_IO_ENABLE_USB_HID
    if(mm_conf_get_as_integer(MM_CONF_DISABLE_TUSB) == 0) {
        MM_CHECK_E(__register_hidconfig() == ESP_OK);
    }
    #endif
    MM_CHECK_E(__register_info() == ESP_OK);
}
