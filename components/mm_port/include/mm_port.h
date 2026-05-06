
#ifndef __MM_PORT_H
#define __MM_PORT_H

#include "mm_err.h"
#include "mm_port_display.h"
#include "mm_port_imu.h"
#include "mm_port_fs.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mm_port_init(void);
int mm_port_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PORT_H */
