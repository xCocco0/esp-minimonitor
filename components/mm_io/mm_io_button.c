/**
 * @file
 *
 * @brief Handle physical buttons
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_io_button.h"

#include "button_gpio.h"
#include "iot_button.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_macros.h"


#define GET_ACTION_CB(prefix) \
    (IS_ENABLED(prefix##_SNOOZE) ? __snooze_cb : \
     IS_ENABLED(prefix##_LEFT)   ? __tile_left_cb : \
     IS_ENABLED(prefix##_RIGHT)  ? __tile_right_cb : \
     IS_ENABLED(prefix##_CYCLE)  ? __tile_cycle_cb : \
     IS_ENABLED(prefix##_HOME)  ? __tile_home_cb : \
     NULL)


static void __unused __snooze_cb(void *arg, void *usr_data)
{
    mm_snooze(NULL);
    mm_toast("Notifications cleared");
}

static void __unused __tile_cycle_cb(void *arg, void *usr_data)
{
    mm_tile_cycle();
}

static void __unused __tile_home_cb(void *arg, void *usr_data)
{
    mm_tile_home();
}

static void __unused __tile_right_cb(void *arg, void *usr_data)
{
    mm_tile_right();
}

static void __unused __tile_left_cb(void *arg, void *usr_data)
{
    mm_tile_left();
}

static void __unused __setup_button(int32_t gpio, button_cb_t single_press_cb, button_cb_t long_press_cb)
{
    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = gpio,
        .active_level = 0,
    };
    button_event_args_t args = {
        .long_press.press_time = CONFIG_MM_IO_LONG_PRESS_TIME_MS,
    };

    button_handle_t gpio_btn = NULL;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &gpio_btn);
    MM_CHECK_E(gpio_btn != NULL);

    if(single_press_cb != NULL) {
        ret = iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, NULL, single_press_cb, NULL);
        MM_CHECK_E(ret == ESP_OK);
    }
    if(long_press_cb != NULL) {
        ret = iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_START, &args, long_press_cb, NULL);
        MM_CHECK_E(ret == ESP_OK);
    }
}

void mm_io_button_init(void)
{
    #if CONFIG_MM_IO_PHYSICAL_BUTTONS > 0 && CONFIG_MM_IO_BTN1_GPIO >= 0
    __setup_button(CONFIG_MM_IO_BTN1_GPIO,
            GET_ACTION_CB(CONFIG_MM_IO_BTN1_SINGLE),
            GET_ACTION_CB(CONFIG_MM_IO_BTN1_LONG));
    #endif

    #if CONFIG_MM_IO_PHYSICAL_BUTTONS > 1 && CONFIG_MM_IO_BTN2_GPIO >= 0
    __setup_button(CONFIG_MM_IO_BTN2_GPIO,
            GET_ACTION_CB(CONFIG_MM_IO_BTN2_SINGLE_ACTION),
            GET_ACTION_CB(CONFIG_MM_IO_BTN2_LONG_ACTION));
    #endif

    #if CONFIG_MM_IO_PHYSICAL_BUTTONS > 2 && CONFIG_MM_IO_BTN3_GPIO >= 0
    __setup_button(CONFIG_MM_IO_BTN3_GPIO,
            GET_ACTION_CB(CONFIG_MM_IO_BTN3_SINGLE_ACTION),
            GET_ACTION_CB(CONFIG_MM_IO_BTN3_LONG_ACTION));
    #endif

}
