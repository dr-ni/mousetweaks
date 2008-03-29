/*
 * Copyright © 2008 Gerd Kohlberger <lowfi@chello.at>
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

#ifndef __MT_CURSOR_H__
#define __MT_CURSOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_CURSOR		  (mt_cursor_get_type ())
#define MT_CURSOR(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MT_TYPE_CURSOR, MtCursor))
#define MT_CURSOR_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MT_TYPE_CURSOR, MtCursorClass))
#define MT_IS_CURSOR(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MT_TYPE_CURSOR))
#define MT_IS_CURSOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MT_TYPE_CURSOR))
#define MT_CURSOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MT_TYPE_CURSOR, MtCursorClass))

typedef struct _MtCursor MtCursor;
typedef struct _MtCursorClass MtCursorClass;

struct _MtCursor {
    GObject parent;
};

struct _MtCursorClass {
    GObjectClass parent;
};

GType mt_cursor_get_type (void) G_GNUC_CONST;

MtCursor *     mt_cursor_new		(const gchar *name,
					 guchar      *image,
					 gushort      width,
					 gushort      height,
					 gushort      xhot,
					 gushort      yhot);
const gchar *  mt_cursor_get_name       (MtCursor    *cursor);
const guchar * mt_cursor_get_image      (MtCursor    *cursor);
guchar *       mt_cursor_get_image_copy (MtCursor    *cursor);
void           mt_cursor_get_hotspot    (MtCursor    *cursor,
					 gushort     *xhot,
					 gushort     *yhot);
void           mt_cursor_get_dimension  (MtCursor    *cursor,
					 gushort     *width,
					 gushort     *height);

G_END_DECLS

#endif /* __MT_CURSOR_H__ */
