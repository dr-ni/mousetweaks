/*
 * Copyright © 2007-2008 Gerd Kohlberger <lowfi@chello.at>
 *
 * This file is part of Mousetweaks.
 *
 * Mousetweaks is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mousetweaks is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MT_MAIN_H__
#define __MT_MAIN_H__

#include <gdk/gdkx.h>
#include <gconf/gconf-client.h>

#include "mt-timer.h"
#include "mt-service.h"
#include "mt-cursor.h"

G_BEGIN_DECLS

typedef struct _MTClosure MTClosure;
struct _MTClosure {
    GConfClient *client;
    GtkBuilder  *ui;

    MtService   *service;
    MtTimer     *delay_timer;
    MtTimer     *dwell_timer;
    MtCursor    *cursor;

    Display *xtst_display;
    gint     n_screens;

    gboolean dwell_drag_started;
    gboolean dwell_gesture_started;
    gboolean override_cursor;
    gboolean move_release;

    gint direction;
    gint pointer_x;
    gint pointer_y;
    gint x_old;
    gint y_old;

    /* options */
    gint     threshold;
    gint     style;
    gboolean delay_enabled;
    gboolean dwell_enabled;
    gboolean dwell_show_ctw;
    gint     dwell_mode;
    gint     dwell_dirs[4];
    gboolean animate_cursor;
};

G_END_DECLS

#endif /* __MT_MAIN_H__ */
