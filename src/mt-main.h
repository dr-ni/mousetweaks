/*
 * Copyright © 2007 Gerd Kohlberger <lowfi@chello.at>
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

#include <dbus/dbus-glib-lowlevel.h>
#include <gconf/gconf-client.h>

#include "mt-timer.h"

G_BEGIN_DECLS

typedef struct _MTClosure MTClosure;
struct _MTClosure {
    DBusConnection *conn;
    GConfClient    *client;

    MtTimer *delay_timer;
    MtTimer *dwell_timer;
    gint     dwell_cct;
    gboolean dwell_drag_started;
    gboolean dwell_gesture_started;
    gboolean override_cursor;

    gint direction;
    gint pointer_x;
    gint pointer_y;

    /* options */
    gint     threshold;
    gint     style;
    gboolean delay_enabled;
    gboolean dwell_enabled;
    gboolean dwell_show_ctw;
    gint     dwell_mode;
    gint     dwell_dirs[4];
};

void spi_shutdown (void);

G_END_DECLS

#endif /* __MT_MAIN_H__ */
