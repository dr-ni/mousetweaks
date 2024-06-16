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

#ifndef __MT_CTW_H__
#define __MT_CTW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

gboolean        mt_ctw_init               (void);

void            mt_ctw_fini               (void);

GtkWidget *     mt_ctw_get_window         (void);

void            mt_ctw_save_geometry      (void);

G_END_DECLS

#endif /* __MT_CTW_H__ */
