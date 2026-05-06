/**
 * @file
 *
 * @brief Main Minimonitor functions
 *
 * Copyright 2025 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "minimonitor.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "mm_event.h"
#include "mm_panic.h"
#include "mm_sensor.h"
#include "mm_persist.h"
#include "mm_conf.h"
#include "mm_macros.h"

static int s_initialized = 0;
static int s_started = 0;

static void __settime_handler(mm_event_t id, void *data)
{
    struct timeval *tv = (struct timeval *) data;
    struct tm timeinfo;

    localtime_r(&tv->tv_sec, &timeinfo);

    if(tv->tv_sec > 0) {
        settimeofday(tv, NULL);
        //mm_ui_set_time(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, tv->tv_usec / 1000);
    }
}
static void __reboot_handler(mm_event_t id, void *data)
{
    esp_restart();
}
static void __settimezone_handler(mm_event_t id, void *data)
{
    char *tz = (char *) data;
    setenv("TZ", tz, 1);
    tzset();
}

static void __check_panic(void)
{
    struct mm_panic_info_s info;
    mm_err_at_startup = mm_err_clear();

    mm_panic_get_info(&info);

    if(mm_panic_info_check(&info) == 0) {
        mm_event_queue_bsod(&info);
    }

}

#if CONFIG_MM_PERIODIC_RAM_REPORT

static void __ram_report_periodic_handler(mm_event_t id, void *data)
{
    static BaseType_t last_activation = 0;
    multi_heap_info_t info;
    BaseType_t tick = xTaskGetTickCount();

    if(!last_activation || tick - last_activation >= pdMS_TO_TICKS(CONFIG_MM_PERIODIC_RAM_REPORT_PERIOD_S*1000)) {
        last_activation = tick;

        heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
        MM_LOG_I("Heap usage: [DEFAULT] %4dkB/%4dkB", info.total_allocated_bytes / 1024, (info.total_allocated_bytes + info.total_free_bytes) / 1024);
        heap_caps_get_info(&info, MALLOC_CAP_DMA);
        MM_LOG_I("            [DMA]     %4dkB/%4dkB", info.total_allocated_bytes / 1024, (info.total_allocated_bytes + info.total_free_bytes) / 1024);
        #if CONFIG_SPIRAM
        heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
        MM_LOG_I("            [SPIRAM]  %4dkB/%4dkB", info.total_allocated_bytes / 1024, (info.total_allocated_bytes + info.total_free_bytes) / 1024);
        #endif
    }
}

#endif /* CONFIG_MM_PERIODIC_RAM_REPORT */

void mm_init(void)
{
    struct timeval tv;
    char *buf;
    struct mm_sensor_meter_config_s config;

    MM_CHECK_E(s_initialized == 0);
    MM_CHECK_E(mm_conf_initialized());

    mm_event_init();
    mm_persist_init();

    /* restore TZ from NVS */
    buf = (char *) mm_conf_get_with_malloc(MM_CONF_TZ);
    setenv("TZ", buf, 1);
    tzset();
    free(buf);

    /* show start time */
    gettimeofday(&tv, NULL);
    mm_settime(&tv);

    mm_event_add_handler(MM_EVENT_SETTIME, __settime_handler);
    mm_event_add_handler(MM_EVENT_REBOOT, __reboot_handler);
    mm_event_add_handler(MM_EVENT_SETTIMEZONE, __settimezone_handler);
    #if CONFIG_MM_PERIODIC_RAM_REPORT
    mm_event_add_handler(MM_EVENT_PERIODIC, __ram_report_periodic_handler);
    #endif

    /* restore sensor meters */
    config.index = 0;
    while(mm_sensor_fetch_meter_config(&config) == MM_OK) {
        mm_event_queue_confsensor(&config);
        config.index++;
    }

    s_initialized = 1;
    MM_LOG_I("Minimonitor initialized");
}

int mm_initialized(void)
{
    return s_initialized;
}

void mm_start(void)
{
    uint8_t log_level;

    __check_panic();
    log_level = mm_conf_get_as_integer(MM_CONF_LOGLEVEL);

    MM_LOG_I("Minimonitor started");
    MM_LOG_I("Switching log level to %d", log_level);
    esp_log_level_set("*", log_level);

    mm_setbacklight(mm_conf_get_as_integer(MM_CONF_BACKLIGHT));

    mm_event_start();

    s_started = 1;
}

int mm_started(void)
{
    return s_started;
}

void mm_stop(void)
{
    /*TODO: not implemented */
    MM_CHECK_E(0);
}

void mm_settime(struct timeval *tv)
{
    mm_event_queue_settime(tv);
}

void mm_gettime(struct tm *timeinfo)
{
    time_t now;

    time(&now);
    localtime_r(&now, timeinfo);
}

void mm_notify(const char *name,
                    const char *icon,
                    uint32_t color,
                    uint32_t duration_ms,
                    const char *msg,
                    mm_notify_stack_mode_t opt,
                    int persistent)
{
    MM_LOG_I("New notification %s", name ? name : "");
    mm_event_queue_notify(name, icon, color, duration_ms, msg, opt, persistent);
}

void mm_toast(const char *msg)
{
    mm_event_queue_notify(NULL, NULL, 0x0, CONFIG_MM_NOTIF_DEFAULT_DURATION, msg, MM_NOTIFY_DEFAULT, 0);
}

void mm_snooze(const char* name)
{
    MM_LOG_I("Snoozing notification(s) %s", name ? name : "");
    mm_event_queue_snooze(name);
}

void mm_setsensor(int index, uint32_t value1, uint32_t value2, uint32_t valueP)
{
    //MM_LOG_I("Received reading for meter %d: (%"PRIu32",%"PRIu32",%"PRIu32")", index, value1, value2, valueP);
    mm_event_queue_setsensor(index, value1, value2, valueP);
}

void mm_confsensor(struct mm_sensor_meter_config_s *config)
{
    char buf[MAX(CONFIG_MM_SENSOR_DESC_FMT_MAX_LEN, CONFIG_MM_SENSOR_PANEL_FMT_MAX_LEN)];

    if(!MM_CHECK_W(config)) return;

    /* parse format strings to accomodate decimal digits */
    strncpy(buf, config->fmt, sizeof(buf));
    mm_sensor_parse_fmt_str(config->fmt, buf, config->decimals, sizeof(config->fmt));
    strncpy(buf, config->panel.fmt, sizeof(buf));
    mm_sensor_parse_fmt_str(config->panel.fmt, buf, 0, sizeof(config->panel.fmt));

    if(strlen(config->label) > 0) {
        MM_CHECK_W(mm_sensor_store_meter_config(config) == MM_OK);
    } else {
        mm_sensor_erase_meter_config(config->index);
    }

    MM_LOG_I("Setting meter config [%d]: Label: %s | Fmt: %s | Max: %u | Timeout: %ums",
            config->index, config->label, config->fmt, config->max_value, config->timeout_ms);
    MM_LOG_I("    | Type: %d | Wakeup: arg=%d, thres=%u | Colors: B:0x%06X W:0x%06X A:0x%06X",
            (int)config->type, config->wakeup_arg, config->wakeup_threshold, config->color.base, config->color.warn, config->color.alert);
    MM_LOG_I("    | Thres: L:%u H:%u arg=%d| Decimals: %d | Panel: thres=%u fmt=%s",
            config->threshold.low, config->threshold.high, config->color_arg, config->decimals, config->panel.wakeup_threshold, config->panel.fmt);

    mm_event_queue_confsensor(config);
}

void mm_reboot(void)
{
    MM_LOG_I("Rebooting");
    mm_event_queue_reboot();
}

void mm_shutdown(void)
{
    MM_LOG_I("Shutting down");
    esp_deep_sleep_start();
}

void mm_sntptrigger(void)
{
    MM_LOG_I("Triggering SNTP sync");
    mm_event_queue_sntptrigger();
}

void mm_sethostname(const char *hostname)
{
    MM_LOG_I("Setting hostname to %s", hostname);
    mm_event_queue_sethostname(hostname);
}

size_t mm_gethostname(const char *hostname, size_t size)
{
    mm_conf_get(MM_CONF_HOSTNAME, (void *) hostname, &size);
    return size;
}

void mm_settimezone(const char *tz)
{
    MM_LOG_I("Setting timezone to %s", tz);
    mm_event_queue_settimezone(tz);
    mm_conf_set(MM_CONF_TZ, tz);
}

void mm_setwifi(const char *ssid, const char *password)
{
    MM_LOG_I("Setting WiFi credentials for SSID %s", ssid);
    mm_event_queue_setwifi(ssid, password);
}

int mm_wifistatus()
{
    // TODO
    printf("wifi status\n");
    return 0;
}

void mm_tile_cycle(void)
{
    MM_LOG_I("Switching to next tile");
    mm_event_queue_changetile(MM_TILE_CYCLE);
}
void mm_tile_left(void)
{
    MM_LOG_I("Switching to tile on the left");
    mm_event_queue_changetile(MM_TILE_SWIPE_LEFT);
}
void mm_tile_right(void)
{
    MM_LOG_I("Switching to tile on the right");
    mm_event_queue_changetile(MM_TILE_SWIPE_RIGHT);
}
void mm_tile_home(void)
{
    MM_LOG_I("Switching to home tile");
    mm_event_queue_changetile(MM_TILE_HOME);
}

void mm_setbacklight(uint8_t bk)
{
    MM_LOG_I("Setting backlight: %"PRIu8"", bk);
    mm_event_queue_setbacklight(bk);
}

void mm_detected_motion(mm_motion_t motion)
{
    MM_LOG_I("Received motion gesture: %d", (int) motion);
    mm_event_queue_motion(motion);
}
