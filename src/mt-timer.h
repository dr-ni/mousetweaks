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

#ifndef __MT_TIMER_H__
#define __MT_TIMER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_TIMER  (mt_timer_get_type ())
#define MT_TIMER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_TIMER, MtTimer))
#define MT_IS_TIMER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_TIMER))

typedef GObjectClass           MtTimerClass;
typedef struct _MtTimer        MtTimer;
typedef struct _MtTimerPrivate MtTimerPrivate;

struct _MtTimer
{
    GObject         parent;
    MtTimerPrivate *priv;
};

GType        mt_timer_get_type        (void) G_GNUC_CONST;

MtTimer *    mt_timer_new             (void);

void         mt_timer_start           (MtTimer *timer);
void         mt_timer_stop            (MtTimer *timer);
gboolean     mt_timer_is_running      (MtTimer *timer);

gdouble      mt_timer_elapsed         (MtTimer *timer);
gdouble      mt_timer_get_target      (MtTimer *timer);
void         mt_timer_set_target      (MtTimer *timer,
                                       gdouble  target);

G_END_DECLS

#endif /* __MT_TIMER_H__ */
