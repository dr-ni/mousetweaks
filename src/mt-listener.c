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

#include "mt-listener.h"

struct _MtListenerPrivate {
    AccessibleEventListener *motion;
    AccessibleEventListener *button;
    AccessibleEventListener *focus;
    Accessible              *current_focus;
};

enum {
    MOTION_EVENT,
    BUTTON_EVENT,
    FOCUS_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void mt_listener_dispose      (GObject               *object);
static void mt_listener_motion_event (const AccessibleEvent *event,
				      gpointer               data);
static void mt_listener_button_event (const AccessibleEvent *event,
				      gpointer               data);
static void mt_listener_focus_event  (const AccessibleEvent *event,
				      gpointer               data);

G_DEFINE_TYPE (MtListener, mt_listener, G_TYPE_OBJECT)

static void
mt_listener_class_init (MtListenerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = mt_listener_dispose;

    signals[MOTION_EVENT] = 
	g_signal_new (g_intern_static_string ("motion_event"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE,
		      1, MT_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[BUTTON_EVENT] = 
	g_signal_new (g_intern_static_string ("button_event"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE,
		      1, MT_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[FOCUS_CHANGED] =
	g_signal_new (g_intern_static_string ("focus_changed"),
		      G_OBJECT_CLASS_TYPE (klass),
		      G_SIGNAL_RUN_LAST,
		      0, NULL, NULL,
		      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private (klass, sizeof (MtListenerPrivate));
}

static void
mt_listener_init (MtListener *listener)
{
    MtListenerPrivate *priv;

    listener->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (listener,
							 MT_TYPE_LISTENER,
							 MtListenerPrivate);

    priv->motion = SPI_createAccessibleEventListener (mt_listener_motion_event,
						      listener);
    SPI_registerGlobalEventListener (priv->motion, "mouse:abs");

    priv->button = SPI_createAccessibleEventListener (mt_listener_button_event,
						      listener);
    SPI_registerGlobalEventListener (priv->button, "mouse:button:");

    priv->focus = SPI_createAccessibleEventListener (mt_listener_focus_event,
						     listener);
    SPI_registerGlobalEventListener (priv->focus, "focus:");
}

static void
mt_listener_dispose (GObject *object)
{
    MtListenerPrivate *priv = MT_LISTENER (object)->priv;

    if (priv->motion) {
	SPI_deregisterGlobalEventListenerAll (priv->motion);
	AccessibleEventListener_unref (priv->motion);
	priv->motion = NULL;
    }
    if (priv->button) {
	SPI_deregisterGlobalEventListenerAll (priv->button);
	AccessibleEventListener_unref (priv->button);
	priv->button = NULL;
    }
    if (priv->focus) {
	SPI_deregisterGlobalEventListenerAll (priv->focus);
	AccessibleEventListener_unref (priv->focus);
	priv->focus = NULL;
    }
    if (priv->current_focus) {
	Accessible_unref (priv->current_focus);
	priv->current_focus = NULL;
    }

    G_OBJECT_CLASS (mt_listener_parent_class)->dispose (object);
}

GType
mt_event_get_type (void)
{
    static GType event = 0;

    if (G_UNLIKELY (event == 0))
	event = g_boxed_type_register_static (g_intern_static_string ("MtEvent"),
					      (GBoxedCopyFunc) mt_event_copy,
					      (GBoxedFreeFunc) mt_event_free);
    return event;
}

MtEvent *
mt_event_copy (const MtEvent *event)
{
    return (MtEvent *) g_memdup (event, sizeof (MtEvent));
}

void
mt_event_free (MtEvent *event)
{
    g_free (event);
}

static void
mt_listener_motion_event (const AccessibleEvent *event, gpointer data)
{
    MtEvent ev;

    ev.type = EV_MOTION;
    ev.x = (gint) event->detail1;
    ev.y = (gint) event->detail2;
    ev.button = 0;

    g_signal_emit (data, signals[MOTION_EVENT], 0, &ev);
}

static void
mt_listener_button_event (const AccessibleEvent *event, gpointer data)
{
    MtEvent ev;

    /*
     * This is obviously dangerous, but it should be
     * guarantied that event-type strings will always
     * be in the form of "mouse:button:[1,2,3][p,r]"
     */
    ev.type = event->type[14] == 'p' ? EV_BUTTON_PRESS : EV_BUTTON_RELEASE;
    ev.x = (gint) event->detail1;
    ev.y = (gint) event->detail2;
    ev.button = event->type[13] == '1' ? 1 : (event->type[13] == '2' ? 2 : 3);

    g_signal_emit (data, signals[BUTTON_EVENT], 0, &ev);
}

static void
mt_listener_focus_event (const AccessibleEvent *event, gpointer data)
{
    MtListenerPrivate *priv = MT_LISTENER (data)->priv;

    if (event->source) {
	if (priv->current_focus)
	    Accessible_unref (priv->current_focus);

	Accessible_ref (event->source);
	priv->current_focus = event->source;

	g_signal_emit (data, signals[FOCUS_CHANGED], 0);
    }
}

MtListener *
mt_listener_get_default (void)
{
    static MtListener *listener = NULL;

    if (!listener)
	listener = g_object_new (MT_TYPE_LISTENER, NULL);

    return listener;
}

Accessible *
mt_listener_current_focus (MtListener *listener)
{
    g_return_val_if_fail (MT_IS_LISTENER (listener), NULL);

    return listener->priv->current_focus;
}
