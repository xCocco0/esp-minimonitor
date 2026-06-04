
#ifndef __MM_PORT_FS_H
#define __MM_PORT_FS_H

#include <stdint.h>
#include "mm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

mm_err_t mm_port_fs_init(void *arg);

int mm_port_fs_available(void);

void *mm_port_fs_get_iohandle(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PORT_FS_H */
