/**
 * @file
 *
 * @brief UI icons
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_icon.h"

#include <stdio.h>
#include <sys/stat.h>
#include "sdkconfig.h"
#include "mm_log.h"

struct __name_and_img {
    const char *name;
    lv_img_dsc_t *img_dsc;
};

static const struct __name_and_img name_and_img[] = {
    {.name = "activity"            , .img_dsc = &mm_icon_activity            },
    {.name = "alarm"               , .img_dsc = &mm_icon_alarm               },
    {.name = "aperture"            , .img_dsc = &mm_icon_aperture            },
    {.name = "arrow"               , .img_dsc = &mm_icon_arrow               },
    {.name = "arrow_down"          , .img_dsc = &mm_icon_arrow_down          },
    {.name = "arrow_left"          , .img_dsc = &mm_icon_arrow_left          },
    {.name = "arrow_right"         , .img_dsc = &mm_icon_arrow_right         },
    {.name = "arrow_up"            , .img_dsc = &mm_icon_arrow_up            },
    {.name = "at"                  , .img_dsc = &mm_icon_at                  },
    {.name = "backward"            , .img_dsc = &mm_icon_backward            },
    {.name = "badge"               , .img_dsc = &mm_icon_badge               },
    {.name = "ban"                 , .img_dsc = &mm_icon_ban                 },
    {.name = "battery"             , .img_dsc = &mm_icon_battery             },
    {.name = "battery_slash"       , .img_dsc = &mm_icon_battery_slash       },
    {.name = "bed"                 , .img_dsc = &mm_icon_bed                 },
    {.name = "bell"                , .img_dsc = &mm_icon_bell                },
    {.name = "bell_slash"          , .img_dsc = &mm_icon_bell_slash          },
    {.name = "bluetooth"           , .img_dsc = &mm_icon_bluetooth           },
    {.name = "bullhorn"            , .img_dsc = &mm_icon_bullhorn            },
    {.name = "cake"                , .img_dsc = &mm_icon_cake                },
    {.name = "checkmark"           , .img_dsc = &mm_icon_checkmark           },
    {.name = "click"               , .img_dsc = &mm_icon_click               },
    {.name = "clock"               , .img_dsc = &mm_icon_clock               },
    {.name = "cloud"               , .img_dsc = &mm_icon_cloud               },
    {.name = "cog"                 , .img_dsc = &mm_icon_cog                 },
    {.name = "double_arrow"        , .img_dsc = &mm_icon_double_arrow        },
    {.name = "double_arrow_slash"  , .img_dsc = &mm_icon_double_arrow_slash  },
    {.name = "empty_sun"           , .img_dsc = &mm_icon_empty_sun           },
    {.name = "envelope"            , .img_dsc = &mm_icon_envelope            },
    {.name = "forward"             , .img_dsc = &mm_icon_forward             },
    {.name = "lock"                , .img_dsc = &mm_icon_lock                },
    {.name = "moon"                , .img_dsc = &mm_icon_moon                },
    {.name = "mouse"               , .img_dsc = &mm_icon_mouse               },
    {.name = "nut"                 , .img_dsc = &mm_icon_nut                 },
    {.name = "phone"               , .img_dsc = &mm_icon_phone               },
    {.name = "play"                , .img_dsc = &mm_icon_play                },
    {.name = "power"               , .img_dsc = &mm_icon_power               },
    {.name = "stopwatch"           , .img_dsc = &mm_icon_stopwatch           },
    {.name = "sun"                 , .img_dsc = &mm_icon_sun                 },
    {.name = "swords"              , .img_dsc = &mm_icon_swords              },
    {.name = "tap"                 , .img_dsc = &mm_icon_tap                 },
    {.name = "touchpad"            , .img_dsc = &mm_icon_touchpad            },
    {.name = "trash"               , .img_dsc = &mm_icon_trash               },
    {.name = "volume_high"         , .img_dsc = &mm_icon_volume_high         },
    {.name = "volume_low"          , .img_dsc = &mm_icon_volume_low          },
    {.name = "volume_mute"         , .img_dsc = &mm_icon_volume_mute         },
    {.name = "whatsapp"            , .img_dsc = &mm_icon_whatsapp            },
    {.name = "wifi"                , .img_dsc = &mm_icon_wifi                },
    {.name = "wifi_slash"          , .img_dsc = &mm_icon_wifi_slash          },
};

static char path_buf[64];

mm_icon_t *mm_icon_search_in_sdcard(const char *name)
{
    #if CONFIG_MM_PORT_HAS_SDCARD
    struct stat st;

    snprintf(path_buf, sizeof(path_buf), "%c:"CONFIG_MM_PORT_SDCARD_MOUNT_POINT"/"CONFIG_MM_PORT_SDCARD_ICON_DIR"/%s.png", LV_FS_STDIO_LETTER, name);

    if(stat(&path_buf[2], &st) == 0) {
        return (mm_icon_t *) path_buf;
    }
    #endif

    return NULL;
}

mm_icon_t *mm_icon_search_in_flash(const char *name)
{
    for(int i = 0; i < sizeof(name_and_img) / sizeof(name_and_img[0]); ++i) {
        if(strcmp(name_and_img[i].name, name) == 0) {
            return (mm_icon_t *) name_and_img[i].img_dsc;
        }
    }
    return NULL;
}

mm_icon_t *mm_icon_by_name(const char *name)
{
    mm_icon_t *ret;

    /* first: search in sdcard */
    ret = mm_icon_search_in_sdcard(name);

    if(ret) {
        return ret;
    }

    /* else: search in flash */
    ret = mm_icon_search_in_flash(name);

    if(ret) {
        return ret;
    }

    MM_LOG_W("Could not find icon \"%s\".", name);

    /* return the default icon */
    return (mm_icon_t *) &mm_icon_bell;
}
