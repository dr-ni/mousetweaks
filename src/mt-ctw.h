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

#ifndef __MT_CTW_H__
#define __MT_CTW_H__

#include "mt-main.h"

G_BEGIN_DECLS

gboolean    mt_ctw_init               (MtData *mt,
				       gint    x,
				       gint    y);
GtkWidget * mt_ctw_get_window         (MtData *mt);
void        mt_ctw_set_clicktype      (MtData *mt,
				       guint   ct);
void        mt_ctw_update_sensitivity (MtData *mt);
void        mt_ctw_update_visibility  (MtData *mt);
void        mt_ctw_update_style       (MtData *mt,
				       gint    style);

G_END_DECLS

#endif /* __MT_CTW_H__ */
