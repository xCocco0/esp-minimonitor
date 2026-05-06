/**
 * @file
 *
 * @brief Handle gyroscope and acceleration sensor
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_io_imu.h"

#include <math.h>
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "minimonitor.h"
#include "mm_macros.h"
#include "mm_port_imu.h"
#include "mm_types.h"
#include "mm_event.h"

#if CONFIG_MM_PORT_HAS_IMU

#define AUTOROT_TASK_CONTINUE (1 << 0)
#define AUTOROT_TASK_STOP     (1 << 1)

static TaskHandle_t s_autorot_task = NULL;

struct coord_s {
    int16_t x;
    int16_t y;
    int16_t z;
};

typedef struct coord_s coord_t;

/* default: A = 20 degrees */
#define TAN_A   CONFIG_MM_IO_IMU_PARAM_TAN_A / CONFIG_MM_IO_IMU_PARAM_SCALE
/* default: B = 45 degrees */
#define TAN_B_2 CONFIG_MM_IO_IMU_PARAM_TAN_B_SQUARE / CONFIG_MM_IO_IMU_PARAM_SCALE

/*
              y  ROT_180
              ^
          ****|****
       ***    |  /  ***
      *       | /      *
     *        |/        *
     *--------+---------->x ROT_270
     *        |         *
ROT_90*       |        *
       ***    |     ***
          **********
             ROT_0
*/

static mm_err_t __compute_rotation(coord_t acc, coord_t gyro, mm_motion_t *out)
{
    const int n = CONFIG_MM_IO_IMU_POLL_PERIOD_MS/6;
    static mm_motion_t last_mot = 0;
    static uint32_t accumulated_ms = 0;
    mm_motion_t mot = 0;

    static coord_t acc_avg = {0,0,0};
    acc_avg.x = acc_avg.x + ((acc.x - acc_avg.x) / n);
    acc_avg.y = acc_avg.y + ((acc.y - acc_avg.y) / n);
    acc_avg.z = acc_avg.z + ((acc.z - acc_avg.z) / n);

    if(       acc_avg.y < 0 && ABS(acc_avg.x) < (-acc_avg.y) * TAN_A) {
        mot = MM_MOTION_ANCHOR_LANDSCAPE;
    } else if(acc_avg.x < 0 && ABS(acc_avg.y) < (-acc_avg.x) * TAN_A) {
        mot = MM_MOTION_ANCHOR_PORTRAIT;
    } else if(acc_avg.y > 0 && ABS(acc_avg.x) < ( acc_avg.y) * TAN_A) {
        mot = MM_MOTION_ANCHOR_LANDSCAPE_INV;
    } else if(acc_avg.x > 0 && ABS(acc_avg.y) < ( acc_avg.x) * TAN_A) {
        mot = MM_MOTION_ANCHOR_PORTRAIT_INV;
    } else {
        mot = -1;
    }
    if(POW2(acc_avg.z) <= (POW2(acc_avg.x) + POW2(acc_avg.y)) * TAN_B_2) {
        /* ok */
    } else {
        mot = -1;
    }

    if(mot == last_mot) {
        accumulated_ms += CONFIG_MM_IO_IMU_POLL_PERIOD_MS;
    } else {
        last_mot = mot;
        accumulated_ms = 0;
    }

    if(mot == -1 || accumulated_ms > CONFIG_MM_IO_IMU_ROTATION_CONFIRM_DELAY_MS) {
        if(mot != *out) {
            *out = mot;
            return MM_OK;
        } else {
            return MM_FAIL;
        }
    } else {
        return MM_FAIL;
    }
}

static void __motion_detection_task(void* arg)
{
    uint32_t cmd;
    int running = 0;
    coord_t acc, gyro;
    mm_motion_t mot = -1;

    for(;;) {
        /* if running: try take notification
           else: wait indefinetely until notification is received */
        if(xTaskNotifyWait(0, AUTOROT_TASK_CONTINUE | AUTOROT_TASK_STOP, &cmd, running ? 0 : portMAX_DELAY) == pdPASS) {
            if(cmd & AUTOROT_TASK_CONTINUE) {
                running = 1;
            } else if(cmd & AUTOROT_TASK_STOP) {
                running = 0;
            }
        }

        if(running) {
            mm_port_imu_read_accel(&acc.x, &acc.y, &acc.z);
            mm_port_imu_read_gyro(&gyro.x, &gyro.y, &gyro.z);

            if(__compute_rotation(acc, gyro, &mot) == MM_OK) {
                if(mot >= 0) {
                    mm_event_queue_motion(mot);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_MM_IO_IMU_POLL_PERIOD_MS));
    }
}

void mm_io_imu_init(void)
{
    xTaskCreate(__motion_detection_task,
            "mm_detect_rotation_task",
            configMINIMAL_STACK_SIZE + CONFIG_MM_IO_ROTATION_TASK_ADDITIONAL_STACK,
            NULL,
            CONFIG_MM_IO_ROTATION_TASK_PRIO,
            &s_autorot_task
    );
}

void mm_io_imu_detect_rotation_start(void)
{
    xTaskNotify(s_autorot_task, AUTOROT_TASK_CONTINUE, eSetBits);
}

void mm_io_imu_detect_rotation_stop(void)
{
    xTaskNotify(s_autorot_task, AUTOROT_TASK_STOP, eSetBits);
}

#else
void mm_io_imu_init(void) {}
void mm_io_imu_detect_rotation_start(void) {}
void mm_io_imu_detect_rotation_stop(void) {}
#endif
