/**
 * @file
 *
 * @brief UI notification management
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MM_UI_NOTIFICATION_H
#define __MM_UI_NOTIFICATION_H

#include "mm_list.h"
#include "mm_widget_notifboard.h"
#include "mm_widget_notiftray.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct mm_notif_s {
    mm_list_node_t list;
    const char *name;
    const char *msg;
    unsigned long id;
    int opt;
    mm_widget_notiftray_icon_t *tray_icon;
    mm_widget_notifboard_entry_t *board_entry;
};

typedef struct mm_notif_s mm_notif_t;

extern MM_LIST_DECLARE(mm_notif_list);

#define MM_NOTIF_PERSISTENT (1 << 0)

#define MM_NOTIF_REPLACE    (1 << 8)
#define MM_NOTIF_STACK      (1 << 9)
#define MM_NOTIF_APPEND     (1 << 10)
#define MM_NOTIF_SILENT     (1 << 11)

#define MM_NOTIF_DEFAULT (MM_NOTIF_STACK)

int mm_ui_notif_post(const char *name, const char* icon_name, const char* msg, int opt);
int mm_ui_notif_cancel_by_name(const char *name);
int mm_ui_notif_cancel_by_id(unsigned long id);
int mm_ui_notif_cancel_all(void);

#ifdef __cplusplus
}
#endif

#endif /* __MM_UI_NOTIFICATION_H */
