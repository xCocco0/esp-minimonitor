/**
 * @file
 *
 * @brief UI notification management
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <limits.h>
#include "mm_err.h"
#include "mm_ui_notification.h"
#include "mm_icon.h"
#include "mm_widgets.h"
#include "mm_ui_state.h"

MM_LIST_DEFINE(mm_notif_list);

static unsigned long s_last_assigned_id = 0;

static mm_notif_t *__mm_notif_create(const char *name,  const char* msg)
{
    struct mm_notif_s *n;
    n = (struct mm_notif_s *) malloc(sizeof(struct mm_notif_s));
    MM_CHECK_MALLOC_E(n);

    n->name = strdup(name);
    MM_CHECK_E(n->name != NULL);
    n->msg = strdup(msg);
    MM_CHECK_E(n->msg != NULL);
    n->id = ++s_last_assigned_id;

    if(s_last_assigned_id == INT_MAX) s_last_assigned_id = 0;

    return n;
}

static void __mm_notif_delete(mm_notif_t *n)
{
    if(n->tray_icon) {
        if(mm_widget_notiftray_icon_sub(n->tray_icon) == 0) {
            /* Notifications with the same name point to the same notif_icon.
             * Only free the last notification */
            mm_widget_notiftray_icon_delete(n->tray_icon);
        }
    }
    mm_widget_notifboard_entry_delete(n->board_entry);
    free((void *) n->name);
    free((void *) n->msg);
    free(n);
}

int mm_ui_notif_post(const char *name, const char* icon_name, const char* msg, int opt)
{
    struct mm_notif_s *n;
    void *icon;
    mm_widget_notiftray_icon_t *nico = NULL;
    int count = 1;

    if(opt == 0) {
        opt = MM_NOTIF_DEFAULT;
    }

    if(opt & MM_NOTIF_REPLACE) {
        (void) mm_ui_notif_cancel_by_name(name);
    }

    n = __mm_notif_create(name, msg);
    n->opt = opt;

    icon = mm_icon_by_name(icon_name);

    /* simply append to list regardless of opt */
    n->board_entry = mm_widget_notifboard_entry_create(mm_widgets.board_notif, n->msg, icon);

    /* if another of the same type exists, we need to do +1 if STACK, otherwise create a new icon */
    if(opt & MM_NOTIF_STACK) {
        MM_LIST_FOREACH_READ(mm_notif_list, i) {
            const mm_notif_t *ic = MM_LIST_ITEM(i, mm_notif_t, list);
            if(strcmp(ic->name, name) == 0) {
                nico = ic->tray_icon;
                break;
            }
        }
    }
    if(nico) {
        n->tray_icon = nico;
        count = mm_widget_notiftray_icon_inc(nico);
    } else if(opt & MM_NOTIF_SILENT) {
        /* if it's not already there, don't create new tray icon */
        n->tray_icon = NULL;
    } else {
        n->tray_icon = mm_widget_notiftray_icon_create(mm_widgets.tray_notif, name, icon, opt);
    }

    MM_LIST_ADD_BACK(mm_notif_list, n->list);

    mm_ui_state.pending_notifications++;

    return count;
}

int mm_ui_notif_cancel_by_name(const char *name)
{
    mm_notif_t *ic;
    int count = 0;

    MM_LIST_FOREACH(mm_notif_list, i) {
        ic = MM_LIST_ITEM(i, mm_notif_t, list);
        if(strcmp(ic->name, name) == 0) {
            MM_LIST_REMOVE(mm_notif_list, i);
            __mm_notif_delete(ic);
            count++;
        }
    }
    mm_ui_state.pending_notifications -= count;
    return count;
}

int mm_ui_notif_cancel_by_id(unsigned long id)
{
    mm_notif_t *ic;

    MM_LIST_FOREACH(mm_notif_list, i) {
        ic = MM_LIST_ITEM(i, mm_notif_t, list);
        if(ic->id == id) {
            MM_LIST_REMOVE(mm_notif_list, i);
            __mm_notif_delete(ic);
            mm_ui_state.pending_notifications -= 1;
            /* ids are unique, we can stop here. */
            return 1;
        }
    }

    return 0;
}

int mm_ui_notif_cancel_all(void)
{
    mm_notif_t *ic;
    int count = 0;

    MM_LIST_FOREACH(mm_notif_list, i) {
        ic = MM_LIST_ITEM(i, mm_notif_t, list);
        if(!(ic->opt & MM_NOTIF_PERSISTENT)) {
            MM_LIST_REMOVE(mm_notif_list, i);
            __mm_notif_delete(ic);
            count++;
        }
    }
    mm_ui_state.pending_notifications -= count;
    return count;
}
