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

#ifndef __MT_ACCESSIBLE_H__
#define __MT_ACCESSIBLE_H__

#include <glib.h>
#include <cspi/spi.h>

G_BEGIN_DECLS

typedef gboolean (*MtSearchFunc) (Accessible *accessible, gpointer data);

typedef enum {
    MT_SEARCH_TYPE_BREADTH = 0,
    MT_SEARCH_TYPE_DEPTH
} MtSearchType;

gboolean     mt_accessible_is_visible      (Accessible  *accessible);
gboolean     mt_accessible_is_active       (Accessible  *accessible);
gboolean     mt_accessible_get_extents     (Accessible  *accessible,
					    SPIRect     *extents);
gboolean     mt_accessible_in_extents      (Accessible  *accessible,
					    gint         x,
					    gint         y);
gboolean     mt_accessible_supports_action (Accessible  *accessible,
					    const gchar *action_name);
Accessible * mt_accessible_search          (Accessible  *accessible,
					    MtSearchType type,
					    MtSearchFunc eval,
					    MtSearchFunc push,
					    gpointer     data);
Accessible * mt_accessible_at_point        (gint         x,
					    gint         y);
gboolean     mt_accessible_point_in_rect   (SPIRect      rectangle,
					    glong        x,
					    glong        y);

G_END_DECLS

#endif /* __MT_ACCESSIBLE_H__ */
