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

#ifndef __MT_CURSOR_MANAGER_H__
#define __MT_CURSOR_MANAGER_H__

#include <glib-object.h>

#include "mt-cursor.h"

G_BEGIN_DECLS

#define MT_TYPE_CURSOR_MANAGER  (mt_cursor_manager_get_type ())
#define MT_CURSOR_MANAGER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_CURSOR_MANAGER, MtCursorManager))
#define MT_IS_CURSOR_MANAGER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_CURSOR_MANAGER))

typedef GObjectClass                   MtCursorManagerClass;
typedef struct _MtCursorManager        MtCursorManager;
typedef struct _MtCursorManagerPrivate MtCursorManagerPrivate;

struct _MtCursorManager
{
    GObject                 parent;
    MtCursorManagerPrivate *priv;
};

GType             mt_cursor_manager_get_type           (void) G_GNUC_CONST;

MtCursorManager * mt_cursor_manager_get_default        (void);
MtCursor *        mt_cursor_manager_get_current_cursor (MtCursorManager *manager);
MtCursor *        mt_cursor_manager_lookup_cursor      (MtCursorManager *manager,
                                                        const gchar     *name);
void              mt_cursor_manager_set_cursor         (MtCursorManager *manager,
                                                        MtCursor        *cursor);
void              mt_cursor_manager_restore_all        (MtCursorManager *manager);

G_END_DECLS

#endif /* __MT_CURSOR_MANAGER_H__ */
