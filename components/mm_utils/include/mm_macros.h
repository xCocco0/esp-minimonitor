/**
 * @file
 *
 * @brief Minimonitor common macro definitions
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_MACROS_H
#define __MM_MACROS_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MIN3
#define MIN3(a,b,c) ((a)<(b)?(MIN(a,c)):(MIN(b,c)))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : (-x))
#endif

#ifndef POW2
#define POW2(x) ((x) * (x))
#endif

#ifndef STR
#define __STR(x) #x
#define STR(x) __STR(x)
#endif

#ifndef IS_ENABLED
#define IS_ENABLED(token) (__STR(token) [0] == '1')
#endif

#ifndef CHOOSE3
#define CHOOSE3(i, a, b, c) ((i) == 0 ? (a) : ((i) == 1 ? (b) : (c)))
#endif

#ifndef GLUE
#define __GLUE(x,y) x##y
#define GLUE(x,y) __GLUE(x,y)
#endif

#ifndef CEIL_DIV
#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#endif

static inline int __USE(int ret, ...) {(void) ret; return ret;}

#ifdef __cplusplus
}
#endif

#endif /* __MM_MACROS_H */
