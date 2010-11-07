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

#include <glib.h>

#include "mt-cursor.h"

struct _MtCursorPrivate
{
    gchar    *name;
    guchar   *image;
    gushort   width;
    gushort   height;
    gushort   xhot;
    gushort   yhot;
};

G_DEFINE_TYPE (MtCursor, mt_cursor, G_TYPE_OBJECT)

static void
mt_cursor_init (MtCursor *cursor)
{
    cursor->priv = G_TYPE_INSTANCE_GET_PRIVATE (cursor,
                                                MT_TYPE_CURSOR,
                                                MtCursorPrivate);
}

static void
mt_cursor_finalize (GObject *object)
{
    MtCursor *cursor = MT_CURSOR (object);

    g_free (cursor->priv->name);
    g_free (cursor->priv->image);

    G_OBJECT_CLASS (mt_cursor_parent_class)->finalize (object);
}

static void
mt_cursor_class_init (MtCursorClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = mt_cursor_finalize;

    g_type_class_add_private (klass, sizeof (MtCursorPrivate));
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

    g_return_val_if_fail (name != NULL, NULL);
    g_return_val_if_fail (image != NULL, NULL);
    g_return_val_if_fail (width > 0 && height > 0, NULL);
    g_return_val_if_fail (xhot <= width && yhot <= height, NULL);

    if (name[0] == '\0')
        return NULL;

    cursor = g_object_new (MT_TYPE_CURSOR, NULL);
    priv = cursor->priv;
    priv->name   = g_strdup (name);
    priv->image  = image;
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

    return cursor->priv->name;
}

const guchar *
mt_cursor_get_image (MtCursor *cursor)
{
    g_return_val_if_fail (MT_IS_CURSOR (cursor), NULL);

    return cursor->priv->image;
}

guchar *
mt_cursor_get_image_copy (MtCursor *cursor)
{
    MtCursorPrivate *priv;

    g_return_val_if_fail (MT_IS_CURSOR (cursor), NULL);

    priv = cursor->priv;

    return g_memdup (priv->image, priv->width * priv->height * 4);
}

void
mt_cursor_get_hotspot (MtCursor *cursor,
                       gushort  *xhot,
                       gushort  *yhot)
{
    g_return_if_fail (MT_IS_CURSOR (cursor));

    if (xhot)
        *xhot = cursor->priv->xhot;
    if (yhot)
        *yhot = cursor->priv->yhot;
}

void
mt_cursor_get_dimension (MtCursor *cursor,
                         gushort  *width,
                         gushort  *height)
{
    g_return_if_fail (MT_IS_CURSOR (cursor));

    if (width)
        *width = cursor->priv->width;
    if (height)
        *height = cursor->priv->height;
}
