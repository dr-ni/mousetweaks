/*
 * Copyright © 2008-2009 Gerd Kohlberger <lowfi@chello.at>
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

#define MT_CURSOR_MANAGER_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MT_TYPE_CURSOR_MANAGER, MtCursorManagerPrivate))

static int fixes_event = 0;
static int fixes_error = 0;

typedef struct _MtCursorManagerPrivate MtCursorManagerPrivate;
struct _MtCursorManagerPrivate {
    GHashTable *cache;
};

enum {
    CURSOR_CHANGED,
    CACHE_CLEARED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MtCursorManager, mt_cursor_manager, G_TYPE_OBJECT)

static void            mt_cursor_manager_finalize    (GObject         *object);
static void            mt_cursor_manager_clear_cache (MtCursorManager *manager);
static GdkFilterReturn xfixes_filter                 (GdkXEvent       *xevent,
						      GdkEvent        *event,
						      gpointer         data);

static void
mt_cursor_manager_class_init (MtCursorManagerClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = mt_cursor_manager_finalize;

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
mt_cursor_manager_init (MtCursorManager *manager)
{
    MtCursorManagerPrivate *priv = MT_CURSOR_MANAGER_GET_PRIVATE (manager);

    priv->cache = NULL;
}

static void
mt_cursor_manager_finalize (GObject *object)
{
    MtCursorManagerPrivate *priv = MT_CURSOR_MANAGER_GET_PRIVATE (object);

    gdk_window_remove_filter (gdk_get_default_root_window (),
			      xfixes_filter, object);

    if (priv->cache)
	g_hash_table_destroy (priv->cache);

    G_OBJECT_CLASS (mt_cursor_manager_parent_class)->finalize (object);
}

static void
mt_cursor_manager_set_xcursor (MtCursor *cursor)
{
    XcursorImage *ximage;
    const gchar  *name;
    Cursor        xcursor;
    gushort       width, height;
    gushort       xhot, yhot;

    mt_cursor_get_dimension (cursor, &width, &height);
    ximage = XcursorImageCreate (width, height);
    xcursor = 0;

    if (ximage) {
	mt_cursor_get_hotspot (cursor, &xhot, &yhot);

	ximage->xhot = xhot;
	ximage->yhot = yhot;
	ximage->delay = 0;
	ximage->pixels = (XcursorPixel *) mt_cursor_get_image (cursor);

	xcursor = XcursorImageLoadCursor (GDK_DISPLAY (), ximage);
	XcursorImageDestroy (ximage);
    }

    if (xcursor) {
	name = mt_cursor_get_name (cursor);

	XFixesSetCursorName (GDK_DISPLAY (), xcursor, name);
	XFixesChangeCursorByName (GDK_DISPLAY (), xcursor, name);
	XFreeCursor (GDK_DISPLAY (), xcursor);

	gdk_flush ();
    }
}

static void
mt_cursor_manager_restore_cursor (gpointer key,
				  gpointer value,
				  gpointer data)
{
    mt_cursor_manager_set_xcursor (MT_CURSOR (value));
}

static void
mt_cursor_manager_add_cursor (MtCursorManager   *manager,
			      XFixesCursorImage *image)
{
    MtCursor *cursor;
    MtCursorManagerPrivate *priv;
    guint32 *copy;
    guchar *pixels;
    guint i, n_pixels;

    /* convert cursor image on x64 arch */
    if (sizeof (unsigned long) != sizeof (guint32)) {
	n_pixels = image->width * image->height;
	copy = g_new (guint32, n_pixels);

	for (i = 0; i < n_pixels; i++)
	    copy[i] = image->pixels[i];

	pixels = (guchar *) copy;
    }
    else
	pixels = (guchar *) image->pixels;


    cursor = mt_cursor_new (image->name, pixels,
			    image->width, image->height,
			    image->xhot, image->yhot);

    if (cursor) {
	priv = MT_CURSOR_MANAGER_GET_PRIVATE (manager);
	g_hash_table_insert (priv->cache, g_strdup (image->name), cursor);
    }
}

static GdkFilterReturn
xfixes_filter (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    XEvent *xev = (XEvent *) xevent;

    if (xev->type == fixes_event + XFixesCursorNotify) {
	XFixesCursorNotifyEvent *cn = (XFixesCursorNotifyEvent *) xev;

	if (cn->cursor_name != None) {
	    MtCursorManager *manager = data;
	    XFixesCursorImage *image;

	    image = XFixesGetCursorImage (GDK_DISPLAY ());
	    if (!mt_cursor_manager_lookup_cursor (manager, image->name))
		mt_cursor_manager_add_cursor (manager, image);

	    g_signal_emit (manager, signals[CURSOR_CHANGED], 0, image->name);
	    XFree (image);
	}
    }
    return GDK_FILTER_CONTINUE;
}

static void
_clear_cursor_cache (GObject    *settings,
		     GParamSpec *pspec,
		     gpointer    data)
{
    mt_cursor_manager_clear_cache (data);
    g_signal_emit (data, signals[CACHE_CLEARED], 0);
}

static MtCursorManager *
mt_cursor_manager_new (void)
{
    MtCursorManager *manager;
    MtCursorManagerPrivate *priv;
    GtkSettings *settings;

    manager = g_object_new (MT_TYPE_CURSOR_MANAGER, NULL);
    priv = MT_CURSOR_MANAGER_GET_PRIVATE (manager);
    priv->cache = g_hash_table_new_full (g_str_hash, g_str_equal,
					 g_free, g_object_unref);

    XFixesQueryExtension (GDK_DISPLAY (), &fixes_event, &fixes_error);
    XFixesSelectCursorInput (GDK_DISPLAY (), GDK_ROOT_WINDOW (),
			     XFixesDisplayCursorNotifyMask);

    gdk_window_add_filter (gdk_get_default_root_window (),
			   xfixes_filter, manager);

    settings = gtk_settings_get_default ();
    g_signal_connect (settings, "notify::gtk-cursor-theme-name",
		      G_CALLBACK (_clear_cursor_cache), manager);
    g_signal_connect (settings, "notify::gtk-cursor-theme-size",
		      G_CALLBACK (_clear_cursor_cache), manager);

    return manager;
}

MtCursorManager *
mt_cursor_manager_get_default (void)
{
    static MtCursorManager *manager = NULL;

    if (manager == NULL)
	manager = mt_cursor_manager_new ();

    return manager;
}

static void
mt_cursor_manager_clear_cache (MtCursorManager *manager)
{
    MtCursorManagerPrivate *priv;

    g_return_if_fail (MT_IS_CURSOR_MANAGER (manager));

    priv = MT_CURSOR_MANAGER_GET_PRIVATE (manager);
    g_hash_table_remove_all (priv->cache);
}

MtCursor *
mt_cursor_manager_current_cursor (MtCursorManager *manager)
{
    XFixesCursorImage *image;
    MtCursor *cursor;

    g_return_val_if_fail (MT_IS_CURSOR_MANAGER (manager), NULL);

    image = XFixesGetCursorImage (GDK_DISPLAY ());
    cursor = NULL;

    if (image->name && image->name[0] != '\0') {
	cursor = mt_cursor_manager_lookup_cursor (manager, image->name);

	if (cursor == NULL) {
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
    MtCursorManagerPrivate *priv;

    g_return_val_if_fail (MT_IS_CURSOR_MANAGER (manager), NULL);

    if (name == NULL || name[0] == '\0')
	return NULL;

    priv = MT_CURSOR_MANAGER_GET_PRIVATE (manager);

    return g_hash_table_lookup (priv->cache, name);
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
    MtCursorManagerPrivate *priv;

    g_return_if_fail (MT_IS_CURSOR_MANAGER (manager));

    priv = MT_CURSOR_MANAGER_GET_PRIVATE (manager);
    g_hash_table_foreach (priv->cache,
			  mt_cursor_manager_restore_cursor,
			  manager);
}
