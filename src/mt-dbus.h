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

#ifndef __MT_DBUS_H__
#define __MT_DBUS_H__

#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>

G_BEGIN_DECLS

#define MOUSETWEAKS_DBUS_SERVICE     "org.freedesktop.mousetweaks"
#define MOUSETWEAKS_DBUS_INTERFACE   "org.freedesktop.mousetweaks"
#define MOUSETWEAKS_DBUS_PATH_MAIN   "/org/freedesktop/mousetweaks/main"
#define MOUSETWEAKS_DBUS_PATH_APPLET "/org/freedesktop/mousetweaks/applet"

#define CLICK_TYPE_SIGNAL  "clicktype"
#define RESTORE_SIGNAL     "restore"
#define ACTIVE_SIGNAL      "active"

DBusConnection * mt_dbus_init	     (gpointer data);

void		 mt_dbus_fini	     (DBusConnection *connection);

void		 mt_dbus_send_signal (DBusConnection *connection,
				      const gchar    *type,
				      gint	     arg);

G_END_DECLS

#endif /* __MT_DBUS_H__ */
