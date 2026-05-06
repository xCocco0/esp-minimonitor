
#include "mm_port.h"

#include "mm_err.h"
#include "sdkconfig.h"
#include "mm_generic_sdcard.h"

mm_err_t mm_port_fs_init(void *arg)
{
    return mm_generic_sdcard_init() == ESP_OK ? MM_OK : MM_FAIL;
}
