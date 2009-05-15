/*
 * Copyright © 2007-2009 Gerd Kohlberger <lowfi@chello.at>
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

typedef struct _MtData MtData;
struct _MtData {
    GConfClient *client;
    GtkBuilder  *ui;
    MtService   *service;
    MtTimer     *delay_timer;
    MtTimer     *dwell_timer;
    MtCursor    *cursor;
    Display     *xtst_display;
    gint         n_screens;
    gint         direction;
    gint         pointer_x;
    gint         pointer_y;
    gint         x_old;
    gint         y_old;

    /* options */
    gint         threshold;
    gint         style;
    gint         dwell_mode;
    gint         dwell_dirs[4];
    guint        delay_enabled  : 1;
    guint        dwell_enabled  : 1;
    guint        dwell_show_ctw : 1;
    guint        animate_cursor : 1;

    /* state flags */
    guint        left_handed           : 1;
    guint        dwell_drag_started    : 1;
    guint        dwell_gesture_started : 1;
    guint        override_cursor       : 1;
    guint        move_release          : 1;
};

G_END_DECLS

#endif /* __MT_MAIN_H__ */
