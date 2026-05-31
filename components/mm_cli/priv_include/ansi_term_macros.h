/**
 * @file
 *
 * @brief Utility macros for the CLI terminal
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ANSI_TERM_MACROS_H
#define __ANSI_TERM_MACROS_H

#define ESC_STR "\x1b"
#define ST_STR "\x9c"

#define QUERY_DEVICE_STATUS_STR ""ESC_STR"[5n"
#define REPORT_DEVICE_OK_STR    ""ESC_STR"[0n"

#define COLOR_STR_3(a, b, c) ESC_STR "[" #a ";" #b ";" #c "m"
#define COLOR_STR_2(a, b)    ESC_STR "[" #a ";" #b "m"
#define COLOR_STR_1(a)       ESC_STR "[" #a "m"

#define COLOR_STR_CHOOSER(_1, _2, _3, NAME, ...) NAME
#define COLOR_STR(...) COLOR_STR_CHOOSER(__VA_ARGS__, COLOR_STR_3, COLOR_STR_2, COLOR_STR_1)(__VA_ARGS__)

/* Text Attributes */
#define ANSI_RESET             0
#define ANSI_BRIGHT            1
#define ANSI_DIM               2
#define ANSI_UNDERSCORE        3
#define ANSI_BLINK             4
#define ANSI_REVERSE           5
#define ANSI_HIDDEN            6

/* Foreground Colors */
#define ANSI_BLACK             30
#define ANSI_RED               31
#define ANSI_GREEN             32
#define ANSI_YELLOW            33
#define ANSI_BLUE              34
#define ANSI_MAGENTA           35
#define ANSI_CYAN              36
#define ANSI_WHITE             37

/* Background Colours */
#define ANSI_BG_BLACK          40
#define ANSI_BG_RED            41
#define ANSI_BG_GREEN          42
#define ANSI_BG_YELLOW         43
#define ANSI_BG_BLUE           44
#define ANSI_BG_MAGENTA        45
#define ANSI_BG_CYAN           46
#define ANSI_BG_WHITE          47

/* Bright Foreground Colors */
#define ANSI_BRIGHT_BLACK      90
#define ANSI_BRIGHT_RED        91
#define ANSI_BRIGHT_GREEN      92
#define ANSI_BRIGHT_YELLOW     93
#define ANSI_BRIGHT_BLUE       94
#define ANSI_BRIGHT_MAGENTA    95
#define ANSI_BRIGHT_CYAN       96
#define ANSI_BRIGHT_WHITE      97

/* Bright Background Colours */
#define ANSI_BG_BRIGHT_BLACK   100
#define ANSI_BG_BRIGHT_RED     101
#define ANSI_BG_BRIGHT_GREEN   102
#define ANSI_BG_BRIGHT_YELLOW  103
#define ANSI_BG_BRIGHT_BLUE    104
#define ANSI_BG_BRIGHT_MAGENTA 105
#define ANSI_BG_BRIGHT_CYAN    106
#define ANSI_BG_BRIGHT_WHITE   107

#endif /* __ANSI_TERM_MACROS_H */
