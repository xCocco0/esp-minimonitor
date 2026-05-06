/**
 * @file
 *
 * @brief Handle ESP panic and coredumps
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */


#include "mm_panic.h"

#include "esp_core_dump.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "mm_err.h"

/**
 * @brief Read the coredump file from flash
 *
 * If return is MM_OK core_dump_data points to a valid memory region with
 * the coredump contents
 */
mm_err_t mm_panic_read_coredump(uint8_t **core_dump_data, size_t *core_dump_size)
{
    MM_LOG_I("Generating core dump...");

    // Find the coredump partition
    const esp_partition_t *core_dump_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");

    MM_LOG_I("Core dump partition: %s, size: %ld", core_dump_partition->label, core_dump_partition->size);

    if(!core_dump_partition) {
        MM_LOG_E("Core dump partition not found!");
        return MM_FAIL;
    }

    // Read the core dump size
    *core_dump_size = core_dump_partition->size;
    *core_dump_data = malloc(*core_dump_size + 1);
    if(!MM_CHECK_MALLOC_W(core_dump_data)) {
        MM_LOG_E("Failed to allocate memory for core dump");
        return MM_FAIL;
    }

    esp_err_t err = esp_partition_read(core_dump_partition, 0, *core_dump_data, *core_dump_size);
    if(err != ESP_OK) {
        MM_LOG_E("Failed to read core dump data: %s", esp_err_to_name(err));
        free(*core_dump_data);
        return MM_FAIL;
    }

    return MM_OK;
}

/**
 * @brief Populate a struct mm_panic_info_s with current panic info
 */
mm_err_t mm_panic_get_info(struct mm_panic_info_s *info)
{
    esp_err_t err;
    esp_core_dump_summary_t *summary = NULL;
    uint32_t cause = 0;
    const esp_reset_reason_t reason = esp_reset_reason();
    info->reset_reason = reason;

    if(reason == ESP_RST_PANIC) {
        summary = malloc(sizeof(esp_core_dump_summary_t));
        if(MM_CHECK_MALLOC_W(summary)) {
            err = esp_core_dump_get_summary(summary);
            if(err != ESP_OK) {
                return MM_FAIL;
            }
            /* app elf sha256 */
            strncpy(info->elf_sha256, (char *) summary->app_elf_sha256, sizeof(info->elf_sha256));
            /* crashed task */
            strncpy(info->crashed_task, (char *) summary->exc_task, sizeof(info->crashed_task));
            /* exception cause*/
            #if CONFIG_IDF_TARGET_ARCH_XTENSA
            cause = summary->ex_info.exc_cause;
            #elif CONFIG_IDF_TARGET_ARCH_RISCV
            cause = summary->ex_info.mcause;
            #endif
            info->cause = cause;
            /* panic reason */
            err = esp_core_dump_get_panic_reason(info->panic_reason, sizeof(info->panic_reason));
            if(err != ESP_OK) {
                info->panic_reason[0] = '\0';
            }
            if(cause == 0 && mm_err_at_startup != 0) {
                info->mm_err = mm_err_at_startup;
            } else {
                info->mm_err = 0;
            }
            free(summary);
            return MM_OK;
        }
    }

    return MM_FAIL;
}

/**
 * @brief Convert a struct mm_panic_info_s into a 6 digits code
 */
mm_err_t mm_panic_info_to_code(const struct mm_panic_info_s *info, char *err)
{
    switch(info->reset_reason) {
        case ESP_RST_UNKNOWN:    // Reset reason can not be determined
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "UNKWN");
            return MM_OK;
        case ESP_RST_INT_WDT:    // Reset (software or hardware) due to interrupt watchdog
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "WD001");
            return MM_OK;
        case ESP_RST_TASK_WDT:   // Reset due to task watchdog
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "WD002");
            return MM_OK;
        case ESP_RST_WDT:        // Reset due to other watchdogs
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "WD003");
            return MM_OK;
        case ESP_RST_EFUSE:      // Reset due to efuse error
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "EFUSE");
            return MM_OK;
        case ESP_RST_PWR_GLITCH: // Reset due to power glitch detected
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "PWRGL");
            return MM_OK;
        case ESP_RST_CPU_LOCKUP: // Reset due to CPU lock up (double exception)
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "DBEXP");
            return MM_OK;
        case ESP_RST_PANIC:      // Software reset due to exception/panic
            break;
        case ESP_RST_POWERON:    // Reset due to power-on event
        case ESP_RST_EXT:        // Reset by external pin (not applicable for ESP32)
        case ESP_RST_SW:         // Software reset via esp_restart
        case ESP_RST_DEEPSLEEP:  // Reset after exiting deep sleep mode
        case ESP_RST_BROWNOUT:   // Brownout reset (software or hardware)
        case ESP_RST_SDIO:       // Reset over SDIO
        case ESP_RST_USB:        // Reset by USB peripheral
        case ESP_RST_JTAG:       // Reset by JTAG
        default:
            /* no apparent errors: return now */
            return MM_FAIL;
    }

    if(info->reset_reason == ESP_RST_PANIC) {
        if(info->cause == 0 && info->mm_err != 0) {
            snprintf(err, MM_PANIC_ERR_CODE_LEN, "MM%03lu", info->mm_err % 1000);
        } else {
            snprintf(err, MM_PANIC_ERR_CODE_LEN,
                #if CONFIG_IDF_TARGET_ARCH_XTENSA
                "XT%03"PRIu8"",
                #elif CONFIG_IDF_TARGET_ARCH_RISCV
                "RV%03"PRIu8"",
                #endif
                (uint8_t) info->cause % 1000);
        }
        return MM_OK;
    }

    return MM_FAIL;
}

/**
 * @brief Check if struct mm_panic_info_s indicates a panic condition
 *
 * @return 0 if panic condition is detected, 1 otherwise
 */
int mm_panic_info_check(const struct mm_panic_info_s *info)
{
    switch(info->reset_reason) {
        case ESP_RST_POWERON:    // Reset due to power-on event
        case ESP_RST_EXT:        // Reset by external pin (not applicable for ESP32)
        case ESP_RST_SW:         // Software reset via esp_restart
        case ESP_RST_DEEPSLEEP:  // Reset after exiting deep sleep mode
        case ESP_RST_BROWNOUT:   // Brownout reset (software or hardware)
        case ESP_RST_SDIO:       // Reset over SDIO
        case ESP_RST_USB:        // Reset by USB peripheral
        case ESP_RST_JTAG:       // Reset by JTAG
            return 1;
        case ESP_RST_UNKNOWN:    // Reset reason can not be determined
        case ESP_RST_INT_WDT:    // Reset (software or hardware) due to interrupt watchdog
        case ESP_RST_TASK_WDT:   // Reset due to task watchdog
        case ESP_RST_WDT:        // Reset due to other watchdogs
        case ESP_RST_EFUSE:      // Reset due to efuse error
        case ESP_RST_PWR_GLITCH: // Reset due to power glitch detected
        case ESP_RST_CPU_LOCKUP: // Reset due to CPU lock up (double exception)
        case ESP_RST_PANIC:      // Software reset due to exception/panic
        default:
            break;
    }

    return 0;
}

