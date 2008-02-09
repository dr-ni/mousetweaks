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

#ifndef __MT_DBUS_H__
#define __MT_DBUS_H__

#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>

G_BEGIN_DECLS

#define MOUSETWEAKS_DBUS_SERVICE     "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_INTERFACE   "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_PATH_MAIN   "/org/gnome/Mousetweaks/Main"
#define MOUSETWEAKS_DBUS_PATH_APPLET "/org/gnome/Mousetweaks/Applet"

#define CLICK_TYPE_SIGNAL  "Clicktype"
#define RESTORE_SIGNAL     "Restore"
#define ACTIVE_SIGNAL      "Active"

DBusConnection * mt_dbus_init	     (gpointer data);

void		 mt_dbus_fini	     (DBusConnection *connection);

void		 mt_dbus_send_signal (DBusConnection *connection,
				      const gchar    *type,
				      gint	     arg);

G_END_DECLS

#endif /* __MT_DBUS_H__ */
