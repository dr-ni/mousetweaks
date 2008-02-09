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

#include <gtk/gtk.h>

#include "mt-main.h"
#include "mt-dbus.h"
#include "mt-common.h"
#include "mt-ctw.h"

static DBusHandlerResult
mt_dbus_filter (DBusConnection *connection,
		DBusMessage *message,
		gpointer data)
{
    MTClosure *mt = (MTClosure *) data;

    if (dbus_message_is_signal (message,
				MOUSETWEAKS_DBUS_INTERFACE,
				CLICK_TYPE_SIGNAL)) {
	if (dbus_message_get_args (message,
				   NULL,
				   DBUS_TYPE_INT32,
				   &mt->dwell_cct,
				   DBUS_TYPE_INVALID)) {
	    mt_ctw_set_click_type (mt->dwell_cct);

	    return DBUS_HANDLER_RESULT_HANDLED;
	}
    }
    else if (dbus_message_is_signal (message,
				     MOUSETWEAKS_DBUS_INTERFACE,
				     ACTIVE_SIGNAL)) {
	mt_dbus_send_signal (connection, ACTIVE_SIGNAL, 1);

	return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
mt_dbus_send_signal (DBusConnection *connection,
		     const gchar *type,
		     gint arg)
{
    DBusMessage *msg;

    msg = dbus_message_new_signal (MOUSETWEAKS_DBUS_PATH_APPLET,
				   MOUSETWEAKS_DBUS_INTERFACE,
				   type);
    dbus_message_append_args (msg,
			      DBUS_TYPE_INT32, &arg,
			      DBUS_TYPE_INVALID);
    dbus_connection_send (connection, msg, NULL);
    dbus_connection_flush (connection);
    dbus_message_unref (msg);
}

DBusConnection *
mt_dbus_init (gpointer data)
{
    DBusConnection *connection;
    DBusError err;
    gint ret;

    dbus_error_init (&err);
    connection = dbus_bus_get (DBUS_BUS_SESSION, &err);

    if (!connection) {
	mt_show_dialog (err.name, err.message, GTK_MESSAGE_ERROR);
	dbus_error_free (&err);
	return NULL;
    }

    dbus_error_free (&err);

    ret = dbus_bus_request_name (connection,
				 MOUSETWEAKS_DBUS_SERVICE,
				 DBUS_NAME_FLAG_DO_NOT_QUEUE,
				 NULL);

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
	dbus_connection_unref (connection);
	return NULL;
    }

    dbus_bus_add_match (connection,
			"type='signal',"
			"interface='"MOUSETWEAKS_DBUS_INTERFACE"',"
			"path='"     MOUSETWEAKS_DBUS_PATH_MAIN"'",
			NULL);

    dbus_connection_add_filter (connection, mt_dbus_filter, data, NULL);
    dbus_connection_setup_with_g_main (connection, NULL);

    return connection;
}

void
mt_dbus_fini (DBusConnection *connection)
{
    dbus_bus_release_name (connection, MOUSETWEAKS_DBUS_SERVICE, NULL);
    dbus_connection_unref (connection);
}
