/*
 * Copyright © 2007-2010 Gerd Kohlberger <gerdko gmail com>
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

#include <gio/gio.h>

#include "mt-service.h"

struct _MtServicePrivate
{
    guint          owner_id;
    GDBusNodeInfo *ispec;

    guint          click_type;
};

enum
{
    PROP_0,
    PROP_CLICK_TYPE
};

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='" MOUSETWEAKS_DBUS_IFACE "'>"
    "    <property type='i' name='ClickType' access='readwrite'/>"
    "  </interface>"
    "</node>";

static void mt_service_bus_acquired (GDBusConnection *connection,
                                     const gchar     *name,
                                     gpointer         data);

G_DEFINE_TYPE (MtService, mt_service, G_TYPE_OBJECT)

static void
mt_service_init (MtService *service)
{
    MtServicePrivate *priv;
    GError *error = NULL;

    service->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (service,
                                                        MT_TYPE_SERVICE,
                                                        MtServicePrivate);

    priv->click_type = MT_DWELL_CLICK_TYPE_SINGLE;
    priv->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                     MOUSETWEAKS_DBUS_NAME,
                                     G_BUS_NAME_OWNER_FLAGS_NONE,
                                     mt_service_bus_acquired,
                                     NULL, NULL,
                                     service, NULL);
    priv->ispec = g_dbus_node_info_new_for_xml (introspection_xml, &error);
    if (error)
    {
        g_warning ("%s\n", error->message);
        g_error_free (error);
    }
}

static void
mt_service_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    MtService *service = MT_SERVICE (object);

    switch (prop_id)
    {
        case PROP_CLICK_TYPE:
            service->priv->click_type = g_value_get_enum (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_service_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    MtService *service = MT_SERVICE (object);

    switch (prop_id)
    {
        case PROP_CLICK_TYPE:
            g_value_set_enum (value, service->priv->click_type);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_service_dispose (GObject *object)
{
    MtServicePrivate *priv = MT_SERVICE (object)->priv;

    if (priv->owner_id)
    {
        g_bus_unown_name (priv->owner_id);
        priv->owner_id = 0;
    }

    if (priv->ispec)
    {
        g_dbus_node_info_unref (priv->ispec);
        priv->ispec = NULL;
    }

    G_OBJECT_CLASS (mt_service_parent_class)->dispose (object);
}

static void
mt_service_class_init (MtServiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = mt_service_get_property;
    object_class->set_property = mt_service_set_property;
    object_class->dispose = mt_service_dispose;

    g_object_class_install_property (object_class,
                                     PROP_CLICK_TYPE,
                                     g_param_spec_enum ("click-type",
                                                        "Click type",
                                                        "The currently active click type",
                                                        MT_TYPE_DWELL_CLICK_TYPE,
                                                        MT_DWELL_CLICK_TYPE_SINGLE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

    g_type_class_add_private (klass, sizeof (MtServicePrivate));
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *path,
                     const gchar     *interface,
                     const gchar     *property,
                     GError         **error,
                     MtService       *service)
{
    GVariant *ret = NULL;

    if (g_strcmp0 (property, "ClickType") == 0)
    {
        ret = g_variant_new_int32 (service->priv->click_type);
    }
    return ret;
}

static gboolean
handle_set_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *path,
                     const gchar     *interface,
                     const gchar     *property,
                     GVariant        *value,
                     GError         **error,
                     MtService       *service)
{
    if (g_strcmp0 (property, "ClickType") == 0)
    {
        mt_service_set_click_type (service, g_variant_get_int32 (value));
    }
    return TRUE;
}

static const GDBusInterfaceVTable interface_vtable =
{
    (GDBusInterfaceMethodCallFunc) NULL,
    (GDBusInterfaceGetPropertyFunc) handle_get_property,
    (GDBusInterfaceSetPropertyFunc) handle_set_property
};

static void
emit_property_changed (GObject         *object,
                       GParamSpec      *pspec,
                       GDBusConnection *connection)
{
    MtService *service = MT_SERVICE (object);
    GError *error = NULL;
    GVariantBuilder builder, inv_builder;
    GVariant *prop_v;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
    g_variant_builder_init (&inv_builder, G_VARIANT_TYPE ("as"));

    if (g_strcmp0 (pspec->name, "click-type") == 0)
    {
        g_variant_builder_add (&builder, "{sv}", "ClickType",
                               g_variant_new_int32 (service->priv->click_type));
    }

    prop_v = g_variant_new ("(sa{sv}as)",
                            MOUSETWEAKS_DBUS_IFACE,
                            &builder, &inv_builder);

    if (!g_dbus_connection_emit_signal (connection, NULL,
                                        MOUSETWEAKS_DBUS_PATH,
                                        "org.freedesktop.DBus.Properties",
                                        "PropertiesChanged",
                                        prop_v, &error))
    {
        g_warning ("%s\n", error->message);
        g_error_free (error);
    }
}

static void
mt_service_bus_acquired (GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         data)
{
    MtService *service = data;

    if (service->priv->ispec)
    {
        GError *error = NULL;

        g_dbus_connection_register_object (connection,
                                           MOUSETWEAKS_DBUS_PATH,
                                           service->priv->ispec->interfaces[0],
                                           &interface_vtable,
                                           service, NULL, &error);
        if (error)
        {
            g_warning ("%s", error->message);
            g_error_free (error);
        }

        g_signal_connect (service, "notify",
                          G_CALLBACK (emit_property_changed), connection);
    }
}

MtService *
mt_service_get_default (void)
{
    static MtService *service = NULL;

    if (!service)
    {
        service = g_object_new (MT_TYPE_SERVICE, NULL);
        g_object_add_weak_pointer (G_OBJECT (service), (gpointer *) &service);
    }
    return service;
}

void
mt_service_set_click_type (MtService       *service,
                           MtDwellClickType type)
{
    g_return_if_fail (MT_IS_SERVICE (service));

    if (type != service->priv->click_type)
    {
        service->priv->click_type = type;
        g_object_notify (G_OBJECT (service), "click-type");
    }
}

MtDwellClickType
mt_service_get_click_type (MtService *service)
{
    g_return_val_if_fail (MT_IS_SERVICE (service), -1);

    return service->priv->click_type;
}
