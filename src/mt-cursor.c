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

#include <glib.h>
#include <string.h>

#include "mt-cursor.h"

#define MT_CURSOR_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MT_TYPE_CURSOR, MtCursorPrivate))

typedef struct _MtCursorPrivate MtCursorPrivate;
struct _MtCursorPrivate {
    gchar  *name;
    guchar *image;
    gushort width;
    gushort height;
    gushort xhot;
    gushort yhot;
};

G_DEFINE_TYPE (MtCursor, mt_cursor, G_TYPE_OBJECT)

static void mt_cursor_finalize (GObject *object);

static void
mt_cursor_class_init (MtCursorClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = mt_cursor_finalize;

    g_type_class_add_private (klass, sizeof (MtCursorPrivate));
}

static void
mt_cursor_init (MtCursor *cursor)
{
    MtCursorPrivate *priv = MT_CURSOR_GET_PRIVATE (cursor);

    priv->name   = NULL;
    priv->image  = NULL;
    priv->width  = 0;
    priv->height = 0;
    priv->xhot   = 0;
    priv->yhot   = 0;
}

static void
mt_cursor_finalize (GObject *object)
{
    MtCursorPrivate *priv = MT_CURSOR_GET_PRIVATE (object);

    if (priv->name)
	g_free (priv->name);

    if (priv->image)
	g_free (priv->image);

    G_OBJECT_CLASS (mt_cursor_parent_class)->finalize (object);
}

static guchar *
mt_cursor_copy_image (guchar *image,
		      gushort width,
		      gushort height)
{
    guchar *copy;
    gulong n_bytes;

    n_bytes = width * height * 4;
    copy = (guchar *) g_try_malloc (n_bytes);

    if (copy)
	memcpy (copy, image, n_bytes);

    return copy;
}

MtCursor *
mt_cursor_new (const gchar *name,
	       guchar      *image,
	       gushort      width,
	       gushort      height,
	       gushort      xhot,
	       gushort      yhot)
{
    MtCursor *cursor;
    MtCursorPrivate *priv;
    guchar *copy;

    g_return_val_if_fail (name != NULL, NULL);
    g_return_val_if_fail (image != NULL, NULL);
    g_return_val_if_fail (width > 0 && height > 0, NULL);
    g_return_val_if_fail (xhot <= width && yhot <= height, NULL);

    if (*name == 0)
	return NULL;

    copy = mt_cursor_copy_image (image, width, height);
    if (!copy)
	return NULL;

    cursor = g_object_new (MT_TYPE_CURSOR, NULL);
    priv = MT_CURSOR_GET_PRIVATE (cursor);

    priv->name   = g_strdup (name);
    priv->image  = copy;
    priv->width  = width;
    priv->height = height;
    priv->xhot   = xhot;
    priv->yhot   = yhot;

    return cursor;
}

const gchar *
mt_cursor_get_name (MtCursor *cursor)
{
    g_return_val_if_fail (MT_IS_CURSOR (cursor), NULL);

    return MT_CURSOR_GET_PRIVATE (cursor)->name;
}

const guchar *
mt_cursor_get_image (MtCursor *cursor)
{
    g_return_val_if_fail (MT_IS_CURSOR (cursor), NULL);

    return MT_CURSOR_GET_PRIVATE (cursor)->image;
}

guchar *
mt_cursor_get_image_copy (MtCursor *cursor)
{
    MtCursorPrivate *priv;
    guchar *image;

    g_return_val_if_fail (MT_IS_CURSOR (cursor), NULL);

    priv = MT_CURSOR_GET_PRIVATE(cursor);
    image = mt_cursor_copy_image (priv->image, priv->width, priv->height);

    return image;
}

void
mt_cursor_get_hotspot (MtCursor *cursor,
		       gushort  *xhot,
		       gushort  *yhot)
{
    MtCursorPrivate *priv;

    g_return_if_fail (MT_IS_CURSOR (cursor));

    priv = MT_CURSOR_GET_PRIVATE (cursor);

    if (xhot != NULL)
	*xhot = priv->xhot;

    if (yhot != NULL)
	*yhot = priv->yhot;
}

void
mt_cursor_get_dimension (MtCursor *cursor,
			 gushort  *width,
			 gushort  *height)
{
    MtCursorPrivate *priv;

    g_return_if_fail (MT_IS_CURSOR (cursor));

    priv = MT_CURSOR_GET_PRIVATE (cursor);

    if (width != NULL)
	*width = priv->width;

    if (height != NULL)
	*height = priv->height;
}
