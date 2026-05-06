/**
 * @file
 *
 * @brief Handle ESP panic and coredumps
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 *
 * @note Error code reference:
 *  | Error | Description |
 *  | ----- | -------------------------------------------------------|
 *  | UNKWN | Reset reason can not be determined |
 *  | WD001 | Reset (software or hardware) due to interrupt watchdog |
 *  | WD002 | Reset due to task watchdog |
 *  | WD003 | Reset due to other watchdogs |
 *  | EFUSE | Reset due to efuse error |
 *  | PWRGL | Reset due to power glitch detected |
 *  | DBEXP | Reset due to CPU lock up (double exception) |
 *  | DBEXP | Reset due to CPU lock up (double exception) |
 *  | XT000 | IllegalInstructionCause |
 *  | XT001 | SyscallCause |
 *  | XT002 | InstructionFetchErrorCause |
 *  | XT003 | LoadStoreErrorCause |
 *  | XT004 | Level1InterruptCause |
 *  | XT005 | AllocaCause |
 *  | XT006 | IntegerDivideByZeroCause |
 *  | XT008 | PrivilegedCause |
 *  | XT009 | LoadStoreAlignmentCause |
 *  | XT012 | InstrPIFDateErrorCause |
 *  | XT013 | LoadStorePIFDataErrorCause |
 *  | XT014 | InstrPIFAddrErrorCause |
 *  | XT015 | LoadStorePIFAddrErrorCause |
 *  | XT016 | InstTLBMissCause |
 *  | XT017 | InstTLBMultiHitCause |
 *  | XT018 | InstFetchPrivilegeCause |
 *  | XT020 | InstFetchProhibitedCause |
 *  | XT024 | LoadStoreTLBMissCause |
 *  | XT025 | LoadStoreTLBMultiHitCause |
 *  | XT026 | LoadStorePrivilegeCause |
 *  | XT028 | LoadProhibitedCause |
 *  | XT029 | StoreProhibitedCause |
 *  | XT032 | Coprocessor0Disabled |
 *  | XT033 | Coprocessor1Disabled |
 *  | XT034 | Coprocessor2Disabled |
 *  | XT035 | Coprocessor3Disabled |
 *  | XT036 | Coprocessor4Disabled |
 *  | XT037 | Coprocessor5Disabled |
 *  | XT038 | Coprocessor6Disabled |
 *  | XT039 | Coprocessor7Disabled |
 *  | MM001 | ... |
 */

#ifndef __MM_PANIC_H
#define __MM_PANIC_H

#include <stdint.h>
#include <stdlib.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MM_PANIC_ERR_CODE_LEN 6

/**
 * @brief Struct to store panic information
 */
struct mm_panic_info_s {
    int reset_reason;
    char elf_sha256[32];
    char crashed_task[16];
    int cause;
    char panic_reason[128];
    uint32_t mm_err;
};

mm_err_t mm_panic_read_coredump(uint8_t **core_dump_data, size_t *core_dump_size);
mm_err_t mm_panic_get_info(struct mm_panic_info_s *info);
mm_err_t mm_panic_info_to_code(const struct mm_panic_info_s *info, char *err);
int mm_panic_info_check(const struct mm_panic_info_s *info);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PANIC_H */
