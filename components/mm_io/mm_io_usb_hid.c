/**
 * @file
 *
 * @brief The task sending HID reports through USB
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_io_usb_hid.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "mm_log.h"
#include "mm_macros.h"
#include "mm_hid_scheduler.h"
#include "mm_io_usb.h"

#define __EV_JOIN 0x2

#if CONFIG_MM_IO_ENABLE_USB_HID

static TaskHandle_t s_task_handle = NULL;

void mm_usb_hid_task_spawn(void)
{
    BaseType_t ret;

    if(s_task_handle) {
        MM_LOG_W("USB HID task is already running. Skipping request.");
        return;
    }

    mm_hid_scheduler_init();

    ret = xTaskCreate(mm_usb_hid_task,
        "mm_usb_hid_task",
        configMINIMAL_STACK_SIZE + CONFIG_MM_IO_USB_HID_TASK_ADDITIONAL_STACK,
        NULL,
        CONFIG_MM_IO_USB_HID_TASK_PRIO,
        &s_task_handle
    );
    MM_CHECK_E(ret == pdPASS);
}

void mm_usb_hid_task_kill(void)
{
    xTaskNotify(s_task_handle, __EV_JOIN, eSetBits);
    MM_LOG_D("Sent join signal to the HID task.");
}

#define __NKRO 6

static struct raw_key_state_s {
    uint8_t kb[__NKRO];
    uint8_t mbuttons;
    uint8_t modifiers;
    uint16_t consumer;
} s_raw_key_state;

static uint8_t *__with_kb(int key_down, uint8_t raw_key)
{
    for(size_t i = 0; i < __NKRO; ++i) {
        if(key_down && s_raw_key_state.kb[i] == 0) {
            s_raw_key_state.kb[i] = raw_key;
            break;
        }
        if(!key_down && s_raw_key_state.kb[i] == raw_key) {
            s_raw_key_state.kb[i] = 0;
        }
    }
    return s_raw_key_state.kb;
}

static uint8_t __with_mbutton(int key_down, uint8_t raw_key)
{
    if(key_down) {
        s_raw_key_state.mbuttons |= raw_key;
    } else {
        s_raw_key_state.mbuttons &= ~raw_key;
    }
    return s_raw_key_state.mbuttons;
}

static uint8_t __with_modifier(int key_down, uint8_t raw_key)
{
    if(key_down) {
        s_raw_key_state.modifiers |= raw_key;
    } else {
        s_raw_key_state.modifiers &= ~raw_key;
    }
    return s_raw_key_state.modifiers;
}

static uint16_t __with_consumer(int key_down, uint16_t raw_key)
{
    s_raw_key_state.consumer = key_down ? raw_key : 0;
    return s_raw_key_state.consumer;
}

/* constraints: max 6 keyboard keys at a time
                max 1 consumer a the time
                mouse buttons and modifiers always ok */

static void __process_hid_action(struct mm_hid_action_s *a)
{
    if(MM_KEY_IS_KEYBOARD(a->key)) {
        mm_usb_send_report_keyboard(s_raw_key_state.modifiers, __with_kb(a->key_down, MM_KEY_TO_VALUE(a->key)));
    } else if(MM_KEY_IS_MOUSE(a->key)) {
        switch(a->key) {
            case MM_KEY_MOUSE_ABS_X:   mm_usb_send_report_mouse(1, s_raw_key_state.mbuttons, a->magnitude, 0, 0, 0); break;
            case MM_KEY_MOUSE_ABS_Y:   mm_usb_send_report_mouse(1, s_raw_key_state.mbuttons, 0, a->magnitude, 0, 0); break;
            case MM_KEY_MOUSE_DELTA_X: mm_usb_send_report_mouse(0, s_raw_key_state.mbuttons, a->magnitude, 0, 0, 0); break;
            case MM_KEY_MOUSE_DELTA_Y: mm_usb_send_report_mouse(0, s_raw_key_state.mbuttons, 0, a->magnitude, 0, 0); break;
            case MM_KEY_MOUSE_WHEEL:   mm_usb_send_report_mouse(0, s_raw_key_state.mbuttons, 0, 0, a->magnitude, 0); break;
            case MM_KEY_MOUSE_PAD:     mm_usb_send_report_mouse(0, s_raw_key_state.mbuttons, 0, 0, 0, a->magnitude); break;
            default:
                /* mouse buttons */
                mm_usb_send_report_mouse(0, __with_mbutton(a->key_down, MM_KEY_TO_VALUE(a->key)), 0, 0, 0, 0);
        }
    } else if(MM_KEY_IS_CONSUMER(a->key)) {
        mm_usb_send_report_consumer(__with_consumer(a->key_down, MM_KEY_TO_VALUE(a->key)));
    } else if(MM_KEY_IS_MODIFIER(a->key)) {
        mm_usb_send_report_keyboard(__with_modifier(a->key_down, MM_KEY_TO_VALUE(a->key)), s_raw_key_state.kb);
    } else {
        /* invalid key: reset HID state */
        memset(&s_raw_key_state, '\0', sizeof(s_raw_key_state));
        mm_usb_send_report_keyboard(0, s_raw_key_state.kb);
        mm_usb_send_report_mouse(0, 0, 0, 0, 0, 0);
        mm_usb_send_report_consumer(0);
    }
}

void mm_usb_hid_task(void *arg)
{
    uint32_t max_wait_ms = 0;
    unsigned long ev_bits;
    struct mm_hid_action_s act;

    for(;;) {

        max_wait_ms = MIN(max_wait_ms, 2000);
        if(mm_hid_scheduler_schedule_next(&act, &max_wait_ms)) {
            MM_LOG_D("Key %s(%"PRIi16") %s", MM_KEY_TO_STR(act.key), act.magnitude, act.key_down ? "DOWN" : "UP");
            MM_LOG_D("hid_task: max_wait_ms=%"PRIu32"", max_wait_ms);
            __process_hid_action(&act);
        }

        xTaskNotifyWait(0, ULONG_MAX, &ev_bits, 0);

        if(ev_bits & __EV_JOIN) {
            break;
        }
    }

    MM_LOG_I("USB HID Task stopped");
}

#else
void mm_usb_hid_task_spawn(void) {}
void mm_usb_hid_task_kill(void) {}
void mm_usb_hid_task(void *arg) {}
#endif
