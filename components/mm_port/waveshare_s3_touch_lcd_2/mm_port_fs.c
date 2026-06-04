
#include "mm_port_fs.h"

#include "sdkconfig.h"
#include "mm_err.h"
#include "sd_protocol_types.h"
#include "mm_generic_sdcard.h"

mm_err_t mm_port_fs_init(void *arg)
{
    if(mm_generic_sdcard_init() == ESP_OK) {
        return MM_OK;
    } else {
        return MM_FAIL;
    }
}

void *mm_port_fs_get_iohandle(void)
{
    return (void *) mm_port_sdcard;
}

