/**
 * @file
 *
 * @brief Handle LED strip interaction
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_io_ledstrip.h"

#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "mm_event.h"


#if CONFIG_MM_IO_HAS_LEDSTRIP

static led_strip_handle_t s_ledstrip;
static uint8_t s_notif_theme[3];
static int s_notif_flashes_left;
static SemaphoreHandle_t s_ledstrip_sem = NULL;

static const uint8_t s_ledstrip_anim_frames[192][3] = {
    {64, 1, 0}, {63, 2, 0}, {62, 3, 0}, {61, 4, 0}, {60, 5, 0}, {59, 6, 0}, {58, 7, 0}, {57, 8, 0},
    {56, 9, 0}, {55, 10, 0}, {54, 11, 0}, {53, 12, 0}, {52, 13, 0}, {51, 14, 0}, {50, 15, 0}, {49, 16, 0},
    {48, 17, 0}, {47, 18, 0}, {46, 19, 0}, {45, 20, 0}, {44, 21, 0}, {43, 22, 0}, {42, 23, 0}, {41, 24, 0},
    {40, 25, 0}, {39, 26, 0}, {38, 27, 0}, {37, 28, 0}, {36, 29, 0}, {35, 30, 0}, {34, 31, 0}, {33, 32, 0},
    {32, 33, 0}, {31, 34, 0}, {30, 35, 0}, {29, 36, 0}, {28, 37, 0}, {27, 38, 0}, {26, 39, 0}, {25, 40, 0},
    {24, 41, 0}, {23, 42, 0}, {22, 43, 0}, {21, 44, 0}, {20, 45, 0}, {19, 46, 0}, {18, 47, 0}, {17, 48, 0},
    {16, 49, 0}, {15, 50, 0}, {14, 51, 0}, {13, 52, 0}, {12, 53, 0}, {11, 54, 0}, {10, 55, 0}, {9, 56, 0},
    {8, 57, 0}, {7, 58, 0}, {6, 59, 0}, {5, 60, 0}, {4, 61, 0}, {3, 62, 0}, {2, 63, 0}, {1, 64, 0},

    {0, 64, 1}, {0, 63, 2}, {0, 62, 3}, {0, 61, 4}, {0, 60, 5}, {0, 59, 6}, {0, 58, 7}, {0, 57, 8},
    {0, 56, 9}, {0, 55, 10}, {0, 54, 11}, {0, 53, 12}, {0, 52, 13}, {0, 51, 14}, {0, 50, 15}, {0, 49, 16},
    {0, 48, 17}, {0, 47, 18}, {0, 46, 19}, {0, 45, 20}, {0, 44, 21}, {0, 43, 22}, {0, 42, 23}, {0, 41, 24},
    {0, 40, 25}, {0, 39, 26}, {0, 38, 27}, {0, 37, 28}, {0, 36, 29}, {0, 35, 30}, {0, 34, 31}, {0, 33, 32},
    {0, 32, 33}, {0, 31, 34}, {0, 30, 35}, {0, 29, 36}, {0, 28, 37}, {0, 27, 38}, {0, 26, 39}, {0, 25, 40},
    {0, 24, 41}, {0, 23, 42}, {0, 22, 43}, {0, 21, 44}, {0, 20, 45}, {0, 19, 46}, {0, 18, 47}, {0, 17, 48},
    {0, 16, 49}, {0, 15, 50}, {0, 14, 51}, {0, 13, 52}, {0, 12, 53}, {0, 11, 54}, {0, 10, 55}, {0, 9, 56},
    {0, 8, 57}, {0, 7, 58}, {0, 6, 59}, {0, 5, 60}, {0, 4, 61}, {0, 3, 62}, {0, 2, 63}, {0, 1, 64},

    {1, 0, 64}, {2, 0, 63}, {3, 0, 62}, {4, 0, 61}, {5, 0, 60}, {6, 0, 59}, {7, 0, 58}, {8, 0, 57},
    {9, 0, 56}, {10, 0, 55}, {11, 0, 54}, {12, 0, 53}, {13, 0, 52}, {14, 0, 51}, {15, 0, 50}, {16, 0, 49},
    {17, 0, 48}, {18, 0, 47}, {19, 0, 46}, {20, 0, 45}, {21, 0, 44}, {22, 0, 43}, {23, 0, 42}, {24, 0, 41},
    {25, 0, 40}, {26, 0, 39}, {27, 0, 38}, {28, 0, 37}, {29, 0, 36}, {30, 0, 35}, {31, 0, 34}, {32, 0, 33},
    {33, 0, 32}, {34, 0, 31}, {35, 0, 30}, {36, 0, 29}, {37, 0, 28}, {38, 0, 27}, {39, 0, 26}, {40, 0, 25},
    {41, 0, 24}, {42, 0, 23}, {43, 0, 22}, {44, 0, 21}, {45, 0, 20}, {46, 0, 19}, {47, 0, 18}, {48, 0, 17},
    {49, 0, 16}, {50, 0, 15}, {51, 0, 14}, {52, 0, 13}, {53, 0, 12}, {54, 0, 11}, {55, 0, 10}, {56, 0, 9},
    {57, 0, 8}, {58, 0, 7}, {59, 0, 6}, {60, 0, 5}, {61, 0, 4}, {62, 0, 3}, {63, 0, 2}, {64, 0, 1}
};

static TimerHandle_t s_led_anim_timer;
static TimerHandle_t s_notif_flash_timer;

static void __ledstrip_set_pixel(int i, const uint8_t *color_v)
{
    led_strip_set_pixel(s_ledstrip, i,
        #if CONFIG_MM_IO_LEDSTRIP_CF_RGB
        color_v[1], color_v[0], color_v[2]
        #else
        color_v[0], color_v[1], color_v[2]
        #endif
    );
}

static void __ledstrip_set_pixel_all(const uint8_t *color_v)
{
    xSemaphoreTake(s_ledstrip_sem, portMAX_DELAY);
    for(int j = 0; j < CONFIG_MM_IO_LEDSTRIP_COUNT; ++j) {
        __ledstrip_set_pixel(j, color_v);
    }
    led_strip_refresh(s_ledstrip);
    xSemaphoreGive(s_ledstrip_sem);
}

static void __ledstrip_anim_timer_cb(TimerHandle_t timer)
{
    static int i = 0;
    __ledstrip_set_pixel_all(s_ledstrip_anim_frames[i]);
    i = (i + 1) % (sizeof(s_ledstrip_anim_frames)/(3*sizeof(s_ledstrip_anim_frames[0][0])));
}

static void __ledstrip_notif_flash_once_cb(TimerHandle_t timer)
{
    if(s_notif_flashes_left-- % 2) {
        xSemaphoreTake(s_ledstrip_sem, portMAX_DELAY);
        led_strip_clear(s_ledstrip);
        xSemaphoreGive(s_ledstrip_sem);
    } else {
        __ledstrip_set_pixel_all(s_notif_theme);
    }

    if(s_notif_flashes_left <= 0) {
        xTimerReset(s_led_anim_timer, portMAX_DELAY);
        xTimerStop(timer, portMAX_DELAY);
    }
}

static void __notify_handler_flash_ledstrip(mm_event_t id, void *data)
{
    BaseType_t ret;
    struct mm_notify_param_s *param = (struct mm_notify_param_s *) data;
    int reps = (param->duration_ms / CONFIG_MM_NOTIF_FLASH_SINGLE_MS);

    if(param->duration_ms == 0 || param->color == 0) {
        return;
    }

    /* stop animation */
    xTimerStop(s_led_anim_timer, portMAX_DELAY);

    /* reps must be even */
    if(reps % 2) reps++;
    s_notif_theme[0] = (param->color >> (8*2)) & 0xff;
    s_notif_theme[1] = (param->color >> (8*1)) & 0xff;
    s_notif_theme[2] = (param->color >> (8*0)) & 0xff;
    s_notif_flashes_left = reps;

    /* start flashing with notification */
    __ledstrip_notif_flash_once_cb(s_notif_flash_timer);
    ret = xTimerStart(s_notif_flash_timer, 0);
    MM_CHECK_E(ret == pdPASS);
}

void mm_io_ledstrip_init(void)
{
    esp_err_t ret;
    BaseType_t retb;

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_MM_IO_LEDSTRIP_GPIO,
        .max_leds = CONFIG_MM_IO_LEDSTRIP_COUNT,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_ledstrip);
    MM_CHECK_E(ret == ESP_OK);

    /* Set all LED off to clear all pixels */
    led_strip_clear(s_ledstrip);

    s_ledstrip_sem = xSemaphoreCreateMutex();

    s_led_anim_timer = xTimerCreate("mm_io_ledstrip_anim_timer",
            pdMS_TO_TICKS(1000/CONFIG_MM_IO_LEDSTRIP_ANIM_SPEED_HZ),
            pdTRUE, 0, __ledstrip_anim_timer_cb);
    MM_CHECK_E(s_led_anim_timer != NULL);

    retb = xTimerStart(s_led_anim_timer, 0);
    MM_CHECK_E(retb == pdPASS);

    s_notif_flash_timer = xTimerCreate("mm_io_ledstrip_notif_flash_timer",
            pdMS_TO_TICKS(CONFIG_MM_NOTIF_FLASH_SINGLE_MS),
            pdTRUE, 0, __ledstrip_notif_flash_once_cb);
    MM_CHECK_E(s_notif_flash_timer != NULL);

    mm_event_add_handler(MM_EVENT_NOTIFY, __notify_handler_flash_ledstrip);
}

#else

void mm_io_ledstrip_init(void) {}

#endif
