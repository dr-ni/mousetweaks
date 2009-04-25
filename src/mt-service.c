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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "mt-service.h"
#include "mt-service-glue.h"

#define MOUSETWEAKS_DBUS_SERVICE "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_PATH    "/org/gnome/Mousetweaks"

struct _MtServicePrivate {
    guint clicktype;
};

enum {
    STATUS_CHANGED,
    CLICKTYPE_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MtService, mt_service, G_TYPE_OBJECT)

static void mt_service_dispose  (GObject   *object);
static void mt_service_register (MtService *service);

static void
mt_service_class_init (MtServiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = mt_service_dispose;

    signals[STATUS_CHANGED] =
	g_signal_new (g_intern_static_string ("status_changed"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__BOOLEAN,
		      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
    signals[CLICKTYPE_CHANGED] =
	g_signal_new (g_intern_static_string ("clicktype_changed"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__UINT,
		      G_TYPE_NONE, 1, G_TYPE_UINT);

    g_type_class_add_private (klass, sizeof (MtServicePrivate));

    dbus_g_object_type_install_info (MT_TYPE_SERVICE,
				     &dbus_glib_mt_service_object_info);
}

static void
mt_service_init (MtService *service)
{
    service->priv = G_TYPE_INSTANCE_GET_PRIVATE (service,
						 MT_TYPE_SERVICE,
						 MtServicePrivate);
    mt_service_register (service);
}

static void
mt_service_dispose (GObject *object)
{
    g_signal_emit (object, signals[STATUS_CHANGED], 0, FALSE);

    G_OBJECT_CLASS (mt_service_parent_class)->dispose (object);
}

static void
mt_service_register (MtService *service)
{
    DBusGConnection *bus;
    DBusGProxy *proxy;
    GError *error = NULL;
    guint result;

    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (bus == NULL) {
	g_warning ("Unable to connect to session bus: %s", error->message);
	g_error_free (error);
	return;
    }

    proxy = dbus_g_proxy_new_for_name (bus,
				       DBUS_SERVICE_DBUS,
				       DBUS_PATH_DBUS,
				       DBUS_INTERFACE_DBUS);

    if (!dbus_g_proxy_call (proxy, "RequestName", &error,
			    G_TYPE_STRING, MOUSETWEAKS_DBUS_SERVICE,
			    G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
			    G_TYPE_INVALID,
			    G_TYPE_UINT, &result,
			    G_TYPE_INVALID)) {
	g_warning ("Unable to acquire name: %s", error->message);
	g_error_free (error);
	g_object_unref (proxy);
	return;
    }

    g_object_unref (proxy);

    if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	dbus_g_connection_register_g_object (bus,
					     MOUSETWEAKS_DBUS_PATH,
					     G_OBJECT (service));
    else
	g_warning ("DBus: Not primary name owner.");
}

static MtService *
mt_service_new (void)
{
    return g_object_new (MT_TYPE_SERVICE, NULL);
}

MtService *
mt_service_get_default (void)
{
    static MtService *service = NULL;

    if (!service) {
	service = mt_service_new ();
	g_signal_emit (service, signals[STATUS_CHANGED], 0, TRUE);
    }

    return service;
}

gboolean
mt_service_set_clicktype (MtService *service,
			  guint      clicktype,
			  GError   **error)
{
    g_return_val_if_fail (MT_IS_SERVICE (service), FALSE);

    service->priv->clicktype = clicktype;

    g_signal_emit (service,
		   signals[CLICKTYPE_CHANGED],
		   0, service->priv->clicktype);
    return TRUE;
}

guint
mt_service_get_clicktype (MtService *service)
{
    g_return_val_if_fail (MT_IS_SERVICE (service), 0);

    return service->priv->clicktype;
}
