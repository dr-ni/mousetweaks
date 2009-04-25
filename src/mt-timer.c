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

#include <glib.h>

#include "mt-timer.h"

struct _MtTimerPrivate {
    GTimer *timer;
    guint   tid;
    gdouble elapsed;
    gdouble target;
};

enum {
    TICK,
    FINISHED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MtTimer, mt_timer, G_TYPE_OBJECT)

static void mt_timer_finalize (GObject *object);

static void
mt_timer_class_init (MtTimerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = mt_timer_finalize;

    signals[TICK] = 
	g_signal_new (g_intern_static_string ("tick"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__DOUBLE,
		      G_TYPE_NONE, 1, G_TYPE_DOUBLE);

    signals[FINISHED] =
	g_signal_new (g_intern_static_string ("finished"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__VOID,
		      G_TYPE_NONE, 0);

    g_type_class_add_private (klass, sizeof (MtTimerPrivate));
}

static void
mt_timer_init (MtTimer *timer)
{
    timer->priv = G_TYPE_INSTANCE_GET_PRIVATE (timer,
					       MT_TYPE_TIMER,
					       MtTimerPrivate);
    timer->priv->timer  = g_timer_new ();
    timer->priv->target = 1.2;
}

static void
mt_timer_finalize (GObject *object)
{
    MtTimer *timer = MT_TIMER (object);

    if (timer->priv->tid)
	g_source_remove (timer->priv->tid);

    g_timer_destroy (timer->priv->timer);

    G_OBJECT_CLASS (mt_timer_parent_class)->finalize (object);
}

static gboolean
mt_timer_check_time (gpointer data)
{
    MtTimer *timer = data;
    MtTimerPrivate *priv = timer->priv;

    priv->elapsed = g_timer_elapsed (priv->timer, NULL);
    g_signal_emit (timer, signals[TICK], 0, priv->elapsed);

    if (priv->elapsed >= priv->target) {
	priv->tid = 0;
	priv->elapsed = 0.0;
	g_signal_emit (timer, signals[FINISHED], 0);

	return FALSE;
    }

    return TRUE;
}

MtTimer *
mt_timer_new (void)
{
    return g_object_new (MT_TYPE_TIMER, NULL);
}

void
mt_timer_start (MtTimer *timer)
{
    g_return_if_fail (MT_IS_TIMER (timer));

    g_timer_start (timer->priv->timer);

    if (timer->priv->tid == 0)
	timer->priv->tid = g_timeout_add (100, mt_timer_check_time, timer);
}

void
mt_timer_stop (MtTimer *timer)
{
    g_return_if_fail (MT_IS_TIMER (timer));

    if (timer->priv->tid) {
	g_source_remove (timer->priv->tid);
	timer->priv->tid = 0;
    }
}

gboolean
mt_timer_is_running (MtTimer *timer)
{
    g_return_val_if_fail (MT_IS_TIMER (timer), FALSE);

    return timer->priv->tid != 0;
}

gdouble
mt_timer_elapsed (MtTimer *timer)
{
    g_return_val_if_fail (MT_IS_TIMER (timer), 0.0);

    return timer->priv->elapsed;
}

gdouble
mt_timer_get_target (MtTimer *timer)
{
    g_return_val_if_fail (MT_IS_TIMER (timer), 0.0);

    return timer->priv->target;
}

void
mt_timer_set_target (MtTimer *timer, gdouble time)
{
    g_return_if_fail (MT_IS_TIMER (timer));
    g_return_if_fail (time >= 0.0);

    timer->priv->target = time;
}
