/*
 * Copyright © 2008-2010 Gerd Kohlberger <gerdko gmail com>
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
#include <gdk/gdkx.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>

#include "mt-cursor.h"
#include "mt-cursor-manager.h"

static int xfixes_event = 0;
static int xfixes_error = 0;

struct _MtCursorManagerPrivate
{
    GHashTable *cache;
};

enum
{
    CURSOR_CHANGED,
    CACHE_CLEARED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void            mt_cursor_manager_clear_cache  (GObject         *settings,
                                                       GParamSpec      *pspec,
                                                       MtCursorManager *manager);
static GdkFilterReturn mt_cursor_manager_event_filter (GdkXEvent       *gdk_xevent,
                                                       GdkEvent        *gdk_event,
                                                       gpointer         data);

G_DEFINE_TYPE (MtCursorManager, mt_cursor_manager, G_TYPE_OBJECT)

static Display *
mt_cursor_manager_display (void)
{
    return GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
}

static void
mt_cursor_manager_init (MtCursorManager *manager)
{
    GtkSettings *gs;

    manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager,
                                                 MT_TYPE_CURSOR_MANAGER,
                                                 MtCursorManagerPrivate);

    manager->priv->cache = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  g_object_unref);

    /* init XFixes extension */
    if (XFixesQueryExtension (mt_cursor_manager_display (),
                              &xfixes_event, &xfixes_error))
    {
        XFixesSelectCursorInput (mt_cursor_manager_display (),
                                 GDK_ROOT_WINDOW (),
                                 XFixesDisplayCursorNotifyMask);
    }

    gdk_window_add_filter (NULL, mt_cursor_manager_event_filter, manager);

    /* listen for cursor theme changes */
    gs = gtk_settings_get_default ();
    g_signal_connect (gs, "notify::gtk-cursor-theme-name",
                      G_CALLBACK (mt_cursor_manager_clear_cache), manager);
    g_signal_connect (gs, "notify::gtk-cursor-theme-size",
                      G_CALLBACK (mt_cursor_manager_clear_cache), manager);
}

static void
mt_cursor_manager_finalize (GObject *object)
{
    MtCursorManagerPrivate *priv = MT_CURSOR_MANAGER (object)->priv;

    gdk_window_remove_filter (NULL, mt_cursor_manager_event_filter, object);
    g_hash_table_destroy (priv->cache);

    G_OBJECT_CLASS (mt_cursor_manager_parent_class)->finalize (object);
}

static void
mt_cursor_manager_class_init (MtCursorManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = mt_cursor_manager_finalize;

    signals[CURSOR_CHANGED] =
        g_signal_new (g_intern_static_string ("cursor_changed"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);

    signals[CACHE_CLEARED] =
        g_signal_new (g_intern_static_string ("cache_cleared"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    g_type_class_add_private (klass, sizeof (MtCursorManagerPrivate));
}

static void
mt_cursor_manager_set_xcursor (MtCursor *cursor)
{
    Display *dpy;
    XcursorImage *ximage;
    Cursor xcursor;
    const gchar *name;
    gushort width, height;
    gushort xhot, yhot;

    dpy = mt_cursor_manager_display ();
    mt_cursor_get_dimension (cursor, &width, &height);
    ximage = XcursorImageCreate (width, height);
    xcursor = 0;

    if (ximage)
    {
        mt_cursor_get_hotspot (cursor, &xhot, &yhot);

        ximage->xhot = xhot;
        ximage->yhot = yhot;
        ximage->delay = 0;
        ximage->pixels = (XcursorPixel *) mt_cursor_get_image (cursor);

        xcursor = XcursorImageLoadCursor (dpy, ximage);
        XcursorImageDestroy (ximage);
    }

    if (xcursor)
    {
        name = mt_cursor_get_name (cursor);

        XFixesSetCursorName (dpy, xcursor, name);
        XFixesChangeCursorByName (dpy, xcursor, name);
        XFreeCursor (dpy, xcursor);

        gdk_flush ();
    }
}

static void
mt_cursor_manager_restore_cursor (gpointer key,
                                  gpointer value,
                                  gpointer data)
{
    mt_cursor_manager_set_xcursor (value);
}

static void
mt_cursor_manager_add_cursor (MtCursorManager   *manager,
                              XFixesCursorImage *image)
{
    MtCursor *cursor;
    guint32 *copy;
    guchar *pixels;
    guint i, n_pixels;

    /* convert cursor image on x64 arch */
    if (sizeof (unsigned long) != sizeof (guint32))
    {
        n_pixels = image->width * image->height;
        copy = g_new (guint32, n_pixels);

        for (i = 0; i < n_pixels; i++)
            copy[i] = image->pixels[i];

        pixels = (guchar *) copy;
    }
    else
    {
        pixels = (guchar *) image->pixels;
    }

    cursor = mt_cursor_new (image->name, pixels,
                            image->width, image->height,
                            image->xhot, image->yhot);
    if (cursor)
    {
        g_hash_table_insert (manager->priv->cache,
                             g_strdup (image->name),
                             cursor);
    }
}

static GdkFilterReturn
mt_cursor_manager_event_filter (GdkXEvent *gdk_xevent,
                                GdkEvent  *gdk_event,
                                gpointer   data)
{
    XEvent *xev = gdk_xevent;

    if (xev->type == xfixes_event + XFixesCursorNotify)
    {
        XFixesCursorNotifyEvent *cn = (XFixesCursorNotifyEvent *) xev;

        if (cn->cursor_name != None)
        {
            MtCursorManager *manager = data;
            XFixesCursorImage *image;

            image = XFixesGetCursorImage (cn->display);
            if (!mt_cursor_manager_lookup_cursor (manager, image->name))
                mt_cursor_manager_add_cursor (manager, image);

            g_signal_emit (manager, signals[CURSOR_CHANGED], 0, image->name);
            XFree (image);
        }
    }
    return GDK_FILTER_CONTINUE;
}

static void
mt_cursor_manager_clear_cache (GObject         *settings,
                               GParamSpec      *pspec,
                               MtCursorManager *manager)
{
    g_hash_table_remove_all (manager->priv->cache);
    g_signal_emit (manager, signals[CACHE_CLEARED], 0);
}

MtCursorManager *
mt_cursor_manager_get_default (void)
{
    static MtCursorManager *manager = NULL;

    if (!manager)
    {
        manager = g_object_new (MT_TYPE_CURSOR_MANAGER, NULL);
        g_object_add_weak_pointer (G_OBJECT (manager), (gpointer *) &manager);
    }
    return manager;
}

MtCursor *
mt_cursor_manager_current_cursor (MtCursorManager *manager)
{
    XFixesCursorImage *image;
    MtCursor *cursor;

    g_return_val_if_fail (MT_IS_CURSOR_MANAGER (manager), NULL);

    image = XFixesGetCursorImage (mt_cursor_manager_display ());
    cursor = NULL;

    if (image->name && image->name[0] != '\0')
    {
        cursor = mt_cursor_manager_lookup_cursor (manager, image->name);
        if (!cursor)
        {
            mt_cursor_manager_add_cursor (manager, image);
            cursor = mt_cursor_manager_lookup_cursor (manager, image->name);
        }
    }
    XFree (image);

    return cursor;
}

MtCursor *
mt_cursor_manager_lookup_cursor (MtCursorManager *manager,
                                 const gchar     *name)
{
    g_return_val_if_fail (MT_IS_CURSOR_MANAGER (manager), NULL);

    if (name == NULL || name[0] == '\0')
        return NULL;

    return g_hash_table_lookup (manager->priv->cache, name);
}

void
mt_cursor_manager_set_cursor (MtCursorManager *manager,
                              MtCursor        *cursor)
{
    g_return_if_fail (MT_IS_CURSOR_MANAGER (manager));
    g_return_if_fail (MT_IS_CURSOR (cursor));

    mt_cursor_manager_set_xcursor (cursor);
}

void
mt_cursor_manager_restore_all (MtCursorManager *manager)
{
    g_return_if_fail (MT_IS_CURSOR_MANAGER (manager));

    g_hash_table_foreach (manager->priv->cache,
                          mt_cursor_manager_restore_cursor,
                          manager);
}
