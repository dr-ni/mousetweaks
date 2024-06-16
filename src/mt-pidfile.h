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

#ifndef __MT_PIDFILE_H__
#define __MT_PIDFILE_H__

G_BEGIN_DECLS

int       mt_pidfile_create       (void);
pid_t     mt_pidfile_is_running   (void);
int       mt_pidfile_kill_wait    (int signal, int sec);
int       mt_pidfile_remove       (void);

G_END_DECLS

#endif /* __MT_PIDFILE_H__ */
