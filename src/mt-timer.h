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

#ifndef __MT_TIMER_H__
#define __MT_TIMER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_TIMER		 (mt_timer_get_type ())
#define MT_TIMER(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MT_TYPE_TIMER, MtTimer))
#define MT_TIMER_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), MT_TYPE_TIMER, MtTimerClass))
#define MT_IS_TIMER(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MT_TYPE_TIMER))
#define MT_IS_TIMER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MT_TYPE_TIMER))
#define MT_TIMER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MT_TYPE_TIMER, MtTimerClass))

typedef struct _MtTimer MtTimer;
typedef struct _MtTimerClass MtTimerClass;

struct _MtTimer {
    GObject parent;
};

struct _MtTimerClass {
    GObjectClass parent;

    void (*tick)     (MtTimer *timer,
		      gdouble  time);
    void (*finished) (MtTimer *timer);
};

GType mt_timer_get_type (void) G_GNUC_CONST;

MtTimer * mt_timer_new        (void);
void      mt_timer_start      (MtTimer *timer);
void      mt_timer_stop       (MtTimer *timer);
gboolean  mt_timer_is_running (MtTimer *timer);
gdouble   mt_timer_elapsed    (MtTimer *timer);
gdouble   mt_timer_get_target (MtTimer *timer);
void      mt_timer_set_target (MtTimer *timer,
			       gdouble  time);

G_END_DECLS

#endif /* __MT_TIMER_H__ */
