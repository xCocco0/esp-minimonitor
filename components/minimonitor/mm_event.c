/**
 * @file
 *
 * @brief Handle minimonitor events
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 *
 * @todo We need to decide who declares events and who can call mm_event_queue.
 * Currently, events are static in mm_event.h and most calls are issued from minimonitor.c,
 * but we can change it in the future.
 */

#include "mm_event.h"

#include <time.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "mm_list.h"
#include "mm_panic.h"
#include "mm_sensor.h"
#include "mm_err.h"

struct mm_event_handler_node {
    mm_list_node_t node;
    mm_event_handler_t func;
};

struct mm_event_generic {
    mm_event_t id;
    void *data;
};

static MM_LIST_DECLARE(s_event_handler_list[MM_EVENT_MAX]);
static QueueHandle_t s_event_queue = NULL;

static void mm_event_task(void *args);

/**
 * @brief Initialize mm events
 */
void mm_event_init(void)
{
    s_event_queue = xQueueCreate(CONFIG_MM_EVENT_QUEUE_LEN, sizeof(struct mm_event_generic));
    MM_CHECK_E(s_event_queue != NULL);

    for(int i = 0; i < MM_EVENT_MAX; ++i) {
        const mm_list_t tmp = MM_LIST_INIT(s_event_handler_list[i]);
        s_event_handler_list[i] = tmp;
    }
}

/**
 * @brief Start event loop
 *
 * @note Events queued before this function is called are stored in the queue until there is enough space
 */
void mm_event_start(void)
{
    BaseType_t err;

    err = xTaskCreate(mm_event_task, "mm_event_task", configMINIMAL_STACK_SIZE + CONFIG_MM_EVENT_TASK_ADDITIONAL_STACK,
            NULL, CONFIG_MM_EVENT_TASK_PRIO, NULL);
    MM_CHECK_E(err == pdPASS);

    mm_event_queue(MM_EVENT_STARTUP, NULL);
}

/**
 * @brief Queue an event for processing
 */
void mm_event_queue(mm_event_t id, void *data)
{
    struct mm_event_generic e = {
        .id = id,
        .data = data,
    };

    BaseType_t err = xQueueSendToBack(s_event_queue, (void *) &e, 0);
    MM_CHECK_W(err == pdPASS && "Skipping event");
}

/**
 * @brief Add an handler for processing an event
 *
 * @note Handlers are called in the order they are added
 */
void mm_event_add_handler(mm_event_t id, mm_event_handler_t func)
{
    struct mm_event_handler_node *n = (struct mm_event_handler_node *) malloc(sizeof(struct mm_event_handler_node));
    MM_CHECK_MALLOC_E(n);

    n->func = func;
    MM_LIST_ADD_BACK(s_event_handler_list[id], n->node);
}

/**
 * @brief Removes an handler previously added with mm_event_add_handler
 * If func is NULL, all handlers will be deleted.
 */
void mm_event_delete_handler(mm_event_t id, mm_event_handler_t func)
{
    MM_LIST_FOREACH(s_event_handler_list[id], it) {
        struct mm_event_handler_node *n = MM_LIST_ITEM(it, struct mm_event_handler_node, node);

        if(func == NULL || n->func == func) {
            MM_LIST_REMOVE(s_event_handler_list[id], it);
            free(n);
        }
    }
}

static void __mm_event_call_all_handlers(mm_event_t id, void *data) {
    MM_LIST_FOREACH(s_event_handler_list[id], it) {
        struct mm_event_handler_node *n = MM_LIST_ITEM(it, struct mm_event_handler_node, node);
        n->func(id, data);
    }
}

/* --- MM_EVENT_SETTIME --- */

/**
 * @brief Queue event to change system time
 */
void mm_event_queue_settime(struct timeval *tv)
{
    struct timeval *data = (struct timeval *) malloc(sizeof(struct timeval));
    MM_CHECK_MALLOC_E(data);
    memcpy(data, tv, sizeof(struct timeval));
    mm_event_queue(MM_EVENT_SETTIME, data);
}

static void __mm_event_cleanup_settime(void *data)
{
    struct timeval *tv = (struct timeval *) data;
    free(tv);
}

/* --- MM_EVENT_NOTIFY --- */

/**
 * @brief Queue event to send a system notification
 */
void mm_event_queue_notify(const char *name,
                                const char *icon,
                                uint32_t color,
                                uint32_t duration_ms,
                                const char *msg,
                                mm_notify_stack_mode_t opt,
                                int persistent)
{
    struct mm_notify_param_s *data = (struct mm_notify_param_s *) malloc(sizeof(struct mm_notify_param_s));
    MM_CHECK_MALLOC_E(data);
    if(name) {
        data->name = strdup(name);
        MM_CHECK_E(data->name != NULL);
    } else {
        data->name = NULL;
    }
    data->icon = icon;
    data->color = color;
    data->duration_ms = duration_ms;
    if(msg) {
        data->msg = strdup(msg);
        MM_CHECK_E(data->msg != NULL);
    } else {
        data->msg = NULL;
    }
    data->opt = opt;
    data->persistent = persistent;
    mm_event_queue(MM_EVENT_NOTIFY, data);
}

static void __mm_event_cleanup_notify(void *data)
{
    struct mm_notify_param_s *param = (struct mm_notify_param_s *) data;

    if(param->name) {
        free((void *) param->name);
    }
    if(param->msg) {
        free((void *) param->msg);
    }
    free(param);
}

/* --- MM_EVENT_SNOOZE --- */

/**
 * @brief Queue event to snooze notifications
 *
 * @param name[in] Name of the notification to snooze, or all notification is NULL
 */
void mm_event_queue_snooze(const char* name)
{
    char *data;

    if(name) {
        data = strdup(name);
        MM_CHECK_E(data != NULL);
    } else {
        data = NULL;
    }
    mm_event_queue(MM_EVENT_SNOOZE, data);
}

static void __mm_event_cleanup_snooze(void *data)
{
    char *name = (char *) data;
    if(name) {
        free(name);
    }
}

/* --- MM_EVENT_SETSENSOR --- */

/**
 * @brief Queue event to update sensor readings
 */
void mm_event_queue_setsensor(int index, uint32_t value1, uint32_t value2, uint32_t valueP)
{
    struct mm_sensor_data_s *data;
    data = (struct mm_sensor_data_s *) malloc(sizeof(struct mm_sensor_data_s));
    MM_CHECK_MALLOC_E(data);

    data->index = index;
    data->value1 = value1;
    data->value2 = value2;
    data->valueP = valueP;

    mm_event_queue(MM_EVENT_SETSENSOR, data);
}

static void __mm_event_cleanup_setsensor(void *data)
{
    struct mm_meter_set_param_s *param = (struct mm_meter_set_param_s *) data;
    free(param);
}

/* --- MM_EVENT_CONFSENSOR --- */

/**
 * @brief Queue event to update meter bar configuration
 */
void mm_event_queue_confsensor(struct mm_sensor_meter_config_s *param)
{
    struct mm_sensor_meter_config_s *data;
    data = (struct mm_sensor_meter_config_s *) malloc(sizeof(struct mm_sensor_meter_config_s));
    MM_CHECK_MALLOC_E(data);

    memcpy(data, param, sizeof(struct mm_sensor_meter_config_s));

    mm_event_queue(MM_EVENT_CONFSENSOR, data);
}

static void __mm_event_cleanup_confsensor(void *data)
{
    struct mm_meter_config_param_s *param = (struct mm_meter_config_param_s *) data;
    free(param);
}

/* --- MM_EVENT_REBOOT --- */

/**
 * @brief Queue event to reboot system
 */
void mm_event_queue_reboot(void)
{
    mm_event_queue(MM_EVENT_REBOOT, NULL);
}

static void __mm_event_cleanup_reboot(void *data)
{
    (void) data;
}

/* --- MM_EVENT_SNTPTRIGGER --- */

/**
 * @brief Queue event to trigger an SNTP time synchronization
 */
void mm_event_queue_sntptrigger(void)
{
    mm_event_queue(MM_EVENT_SNTPTRIGGER, NULL);
}

static void __mm_event_cleanup_sntptrigger(void *data)
{
    (void) data;
}

/* --- MM_EVENT_SETHOSTNAME --- */

/**
 * @brief Queue event to update system hostname
 */
void mm_event_queue_sethostname(const char *hostname)
{
    char *data = strdup(hostname);
    MM_CHECK_E(data != NULL);
    mm_event_queue(MM_EVENT_SETHOSTNAME, data);
}

static void __mm_event_cleanup_sethostname(void *data)
{
    char *hostname = (char *) data;
    free(hostname);
}

/* --- MM_EVENT_SETTIMEZONE --- */

/**
 * @brief Queue event to update timezone
 */
void mm_event_queue_settimezone(const char *tz)
{
    char *data = strdup(tz);
    MM_CHECK_E(data != NULL);
    mm_event_queue(MM_EVENT_SETTIMEZONE, data);
}

static void __mm_event_cleanup_settimezone(void *data)
{
    char *tz = (char *) data;
    free(tz);
}

/* --- MM_EVENT_SETWIFI --- */

/**
 * @brief Queue event to update wifi credentials
 */
void mm_event_queue_setwifi(const char *ssid, const char *password)
{
    struct mm_setwifi_param_s *data = (struct mm_setwifi_param_s *) malloc(sizeof(struct mm_setwifi_param_s));
    MM_CHECK_MALLOC_E(data);

    if(ssid) {
        data->ssid = strdup(ssid);
        MM_CHECK_E(data->ssid != NULL);
    } else {
        data->ssid = NULL;
    }

    if(password) {
        data->password = strdup(password);
        MM_CHECK_E(data->password != NULL);
    } else {
        data->password = NULL;
    }

    mm_event_queue(MM_EVENT_SETWIFI, data);
}

static void __mm_event_cleanup_setwifi(void *data)
{
    struct mm_setwifi_param_s *param = (struct mm_setwifi_param_s *) data;
    if(param->ssid) {
        free((void *) param->ssid);
    }
    if(param->password) {
        free((void *) param->password);
    }
    free(param);
}

/* --- MM_EVENT_CHANGETILE --- */

/**
 * @brief Queue event to switch ui tile
 */
void mm_event_queue_changetile(mm_tile_command_t cmd)
{
    mm_event_queue(MM_EVENT_CHANGETILE, (void *) cmd);
}

static void __mm_event_cleanup_changetile(void *data)
{
    (void) data;
}

/* --- MM_EVENT_SETBACKLIGHT --- */

/**
 * @brief Queue event to update backlight level
 */
void mm_event_queue_setbacklight(uint8_t bk)
{
    mm_event_queue(MM_EVENT_SETBACKLIGHT, (void *)(size_t) bk);
}

static void __mm_event_cleanup_setbacklight(void *data)
{
    (void) data;
}

/* --- MM_EVENT_SETBACKLIGHT --- */

/**
 * @brief Queue event to trigger a panic screen
 */
void mm_event_queue_bsod(struct mm_panic_info_s *param)
{
    struct mm_panic_info_s *data = (struct mm_panic_info_s *) malloc(sizeof(struct mm_panic_info_s));
    MM_CHECK_MALLOC_E(data);

    memcpy(data, param, sizeof(struct mm_panic_info_s));
    mm_event_queue(MM_EVENT_BSOD, (void *) data);
}

static void __mm_event_cleanup_bsod(void *data)
{
    free(data);
}

/* --- MM_EVENT_MOTION --- */

/**
 * @brief Queue event to change screen orientation
 *
 * @param rotation[in] Screen rotation in degrees
 */
void mm_event_queue_motion(mm_motion_t motion)
{
    mm_event_queue(MM_EVENT_MOTION, (void *) motion);
}

static void __mm_event_cleanup_motion(void *data)
{
    (void) data;
}

/* --- MM_EVENT_CONFHID --- */

/**
 * @brief Queue event to configure HID UI section
 */

void mm_event_queue_confhid(struct mm_hid_control_config_s *param)
{
    struct mm_hid_control_config_s *data = (struct mm_hid_control_config_s *) malloc(sizeof(struct mm_hid_control_config_s));
    MM_CHECK_MALLOC_E(data);

    memcpy(data, param, sizeof(struct mm_hid_control_config_s));
    mm_event_queue(MM_EVENT_CONFHID, (void *) data);
}

static void __mm_event_cleanup_confhid(void *data)
{
    free(data);
}

/* --- MM_EVENT_SENDHID --- */

/**
 * @brief Queue event to send HID report
 */
void mm_event_queue_sendhid(struct mm_hid_action_s *param)
{
    struct mm_hid_action_s *data = (struct mm_hid_action_s *) malloc(sizeof(struct mm_hid_action_s));
    MM_CHECK_MALLOC_E(data);

    memcpy(data, param, sizeof(struct mm_hid_action_s));
    mm_event_queue(MM_EVENT_SENDHID, (void *) data);
}

static void __mm_event_cleanup_sendhid(void *data)
{
    free(data);
}

/* --- MM_EVENT_DATA_REQUEST/REPLY --- */

/**
 * @brief Queue event to request data
 */
void mm_event_queue_data_request(struct mm_data_request_s *param)
{
    struct mm_data_request_s *data = (struct mm_data_request_s *) malloc(sizeof(struct mm_data_request_s));
    MM_CHECK_MALLOC_E(data);

    memcpy(data, param, sizeof(struct mm_data_request_s));
    mm_event_queue(MM_EVENT_DATA_REQUEST, (void *) data);
}

static void __mm_event_cleanup_data_request(void *data)
{
    struct mm_data_request_s *d = (struct mm_data_request_s *) data;

    if(MM_LIST_LEN(s_event_handler_list[MM_EVENT_DATA_REQUEST]) == 0) {
        /* no handlers available to fill data */
        /* note: this check must be here and not in mm_event_queue_data_request
                 because this is synchronous with the handler callback. */
        *d->size = 0;
    }

    /* note: we always give the samephore so even if no handlers
       are implemented we won't get stuck waiting on the semaphore. */
    if(d->ready_sema) {
        xSemaphoreGive(d->ready_sema);
    }

    free(d);
}

/* --- */

static void __mm_event_cleanup(mm_event_t id, void *data)
{
    switch(id) {
        case MM_EVENT_SETTIME:         __mm_event_cleanup_settime(data);         break;
        case MM_EVENT_NOTIFY:          __mm_event_cleanup_notify(data);          break;
        case MM_EVENT_SNOOZE:          __mm_event_cleanup_snooze(data);          break;
        case MM_EVENT_SETSENSOR:       __mm_event_cleanup_setsensor(data);       break;
        case MM_EVENT_CONFSENSOR:      __mm_event_cleanup_confsensor(data);      break;
        case MM_EVENT_REBOOT:          __mm_event_cleanup_reboot(data);          break;
        case MM_EVENT_SNTPTRIGGER:     __mm_event_cleanup_sntptrigger(data);     break;
        case MM_EVENT_SETHOSTNAME:     __mm_event_cleanup_sethostname(data);     break;
        case MM_EVENT_SETTIMEZONE:     __mm_event_cleanup_settimezone(data);     break;
        case MM_EVENT_SETWIFI:         __mm_event_cleanup_setwifi(data);         break;
        case MM_EVENT_CHANGETILE:      __mm_event_cleanup_changetile(data);      break;
        case MM_EVENT_SETBACKLIGHT:    __mm_event_cleanup_setbacklight(data);    break;
        case MM_EVENT_BSOD:            __mm_event_cleanup_bsod(data);            break;
        case MM_EVENT_MOTION:          __mm_event_cleanup_motion(data);          break;
        case MM_EVENT_CONFHID:         __mm_event_cleanup_confhid(data);         break;
        case MM_EVENT_SENDHID:         __mm_event_cleanup_sendhid(data);         break;
        case MM_EVENT_DATA_REQUEST:    __mm_event_cleanup_data_request(data);    break;
        default: break; /* default also covers EVENT_PERIODIC and EVENT_STARTUP */
    }
}

#define __DELTA_T pdMS_TO_TICKS(CONFIG_MM_EVENT_TASK_PERIOD_MS)

static void mm_event_task(void *args)
{
    BaseType_t ret;
    struct mm_event_generic e;
    uint32_t next_event;
    TickType_t wait_start;

    for(;;) {

        if(MM_LIST_LEN(s_event_handler_list[MM_EVENT_BACKGROUND]) > 0) {
            /* busy wait */
            wait_start = xTaskGetTickCount();
            /* we lower priority to avoid starvation */
            vTaskPrioritySet(NULL, 1);

            while(1) {
                if(xTaskGetTickCount() - wait_start > __DELTA_T) {
                    /* handle MM_EVENT_PERIODIC events */
                    ret = pdFALSE;
                    break;
                }
                if(xQueueReceive(s_event_queue, &e, 0) != pdTRUE) {
                    /* run background handlers */
                    __mm_event_call_all_handlers(MM_EVENT_BACKGROUND, NULL);
                } else {
                    /* process received event */
                    ret = pdTRUE;
                    break;
                }
            }

            vTaskPrioritySet(NULL, CONFIG_MM_EVENT_TASK_PRIO);

        } else {
            /* blocking wait */
            ret = xQueueReceive(s_event_queue, &e, __DELTA_T);
        }

        if(ret == pdPASS) {
            /* received event */

            do {
                /* process events */
                __mm_event_call_all_handlers(e.id, e.data);
                __mm_event_cleanup(e.id, e.data);

            } while(
                /* process all events in queue */
                xQueueReceive(s_event_queue, &e, 0)
            );
        }

        /* periodic timeout */
        __mm_event_call_all_handlers(MM_EVENT_PERIODIC, &next_event);
    }
}
