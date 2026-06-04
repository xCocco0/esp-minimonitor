
#ifndef __MM_GENERIC_SDCARD_H
#define __MM_GENERIC_SDCARD_H

#include "sd_protocol_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern sdmmc_card_t *mm_port_sdcard;

esp_err_t mm_generic_sdcard_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_GENERIC_SDCARD_H */
