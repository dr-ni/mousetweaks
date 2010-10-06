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

#include <gdk/gdkx.h>

#include "mt-listener.h"

#define XBUTTON_MASK (Button1Mask | Button2Mask | Button3Mask | \
                      Button4Mask | Button5Mask)

#define POLL_INTERVAL_IDLE    100
#define POLL_INTERVAL_MOVING  20

struct _MtListenerPrivate
{
    gint  last_x;
    gint  last_y;
    guint mask_state;
};

enum
{
    MOTION_EVENT,
    BUTTON_EVENT,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void              mt_listener_grab_buttons     (void);
static void              mt_listener_ungrab_buttons   (void);
static gboolean          mt_listener_mouse_idle       (gpointer   data);
static GdkFilterReturn   mt_listener_event_filter     (GdkXEvent *gdk_xevent,
                                                       GdkEvent  *gdk_event,
                                                       gpointer   data);

G_DEFINE_TYPE (MtListener, mt_listener, G_TYPE_OBJECT)

static void
mt_listener_init (MtListener *listener)
{
    listener->priv = G_TYPE_INSTANCE_GET_PRIVATE (listener,
                                                  MT_TYPE_LISTENER,
                                                  MtListenerPrivate);

    gdk_window_add_filter (NULL, mt_listener_event_filter, listener);
    mt_listener_grab_buttons ();
    mt_listener_mouse_idle (listener);
}

static void
mt_listener_finalize (GObject *object)
{
    gdk_window_remove_filter (NULL, mt_listener_event_filter, object);
    mt_listener_ungrab_buttons ();

    G_OBJECT_CLASS (mt_listener_parent_class)->finalize (object);
}

static void
mt_listener_class_init (MtListenerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = mt_listener_finalize;

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

    g_type_class_add_private (klass, sizeof (MtListenerPrivate));
}

static MtEvent *
mt_event_copy (const MtEvent *event)
{
    return g_memdup (event, sizeof (MtEvent));
}

static void
mt_event_free (MtEvent *event)
{
    g_free (event);
}

GType
mt_event_get_type (void)
{
    static GType event = 0;

    if (G_UNLIKELY (event == 0))
    {
        event = g_boxed_type_register_static (g_intern_static_string ("MtEvent"),
                                              (GBoxedCopyFunc) mt_event_copy,
                                              (GBoxedFreeFunc) mt_event_free);
    }
    return event;
}

static void
mt_listener_emit_button_event (MtListener *listener,
                               MtEventType type,
                               gint        button,
                               gint        x,
                               gint        y)
{
    MtEvent ev;

    ev.type = type;
    ev.button = button;
    ev.x = x;
    ev.y = y;

    g_signal_emit (listener, signals[BUTTON_EVENT], 0, &ev);
}

static void
mt_listener_emit_motion_event (MtListener *listener,
                               gint        x,
                               gint        y)
{
    MtEvent ev;

    ev.type = MT_EVENT_MOTION;
    ev.button = 0;
    ev.x = x;
    ev.y = y;

    g_signal_emit (listener, signals[MOTION_EVENT], 0, &ev);
}

static Display *
mt_listener_display (void)
{
    return GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
}

static gboolean
mt_listener_maybe_emit (MtListener *listener, guint mask)
{
    MtListenerPrivate *priv = listener->priv;
    gint button = 0;
    gboolean pressed = FALSE;

    if ((mask & XBUTTON_MASK) == (priv->mask_state & XBUTTON_MASK))
        return FALSE;

    if (!(mask & Button1Mask) && (priv->mask_state & Button1Mask))
    {
        priv->mask_state &= ~Button1Mask;
        button = 1;
    }
    else if ((mask & Button1Mask) && !(priv->mask_state & Button1Mask))
    {
        priv->mask_state |= Button1Mask;
        button = 1;
        pressed = TRUE;
    }
    else if (!(mask & Button2Mask) && (priv->mask_state & Button2Mask))
    {
        priv->mask_state &= ~Button2Mask;
        button = 2;
    }
    else if ((mask & Button2Mask) && !(priv->mask_state & Button2Mask))
    {
        priv->mask_state |= Button2Mask;
        button = 2;
        pressed = TRUE;
    }
    else if (!(mask & Button3Mask) && (priv->mask_state & Button3Mask))
    {
        priv->mask_state &= ~Button3Mask;
        button = 3;
    }
    else if ((mask & Button3Mask) && !(priv->mask_state & Button3Mask))
    {
        priv->mask_state |= Button3Mask;
        button = 3;
        pressed = TRUE;
    }
    /*
     * scroll-wheel events come in press-release pairs,
     * so this is mostly an exercise in futility.
     */
    else if (!(mask & Button4Mask) && (priv->mask_state & Button4Mask))
    {
        priv->mask_state &= ~Button4Mask;
        button = 4;
    }
    else if ((mask & Button4Mask) && !(priv->mask_state & Button4Mask))
    {
        priv->mask_state |= Button4Mask;
        button = 4;
        pressed = TRUE;
    }
    else if (!(mask & Button5Mask) && (priv->mask_state & Button5Mask))
    {
        priv->mask_state &= ~Button5Mask;
        button = 5;
    }
    else if ((mask & Button5Mask) && !(priv->mask_state & Button5Mask))
    {
        priv->mask_state |= Button5Mask;
        button = 5;
        pressed = TRUE;
    }

    if (button)
    {
        mt_listener_emit_button_event (listener,
                                       pressed ? MT_EVENT_BUTTON_PRESS :
                                                 MT_EVENT_BUTTON_RELEASE,
                                       button,
                                       priv->last_x,
                                       priv->last_y);
    }
    return TRUE;
}

static gboolean
mt_listener_mouse_moved (MtListener *listener)
{
    MtListenerPrivate *priv = listener->priv;
    Display *dpy;
    Window root_return, child_return;
    gint x, y, win_x_return, win_y_return;
    guint mask_return;

    dpy = mt_listener_display ();

    gdk_error_trap_push ();
    XQueryPointer (dpy, DefaultRootWindow (dpy),
                   &root_return, &child_return, &x, &y,
                   &win_x_return, &win_y_return, &mask_return);
    gdk_error_trap_pop ();

    while (mt_listener_maybe_emit (listener, mask_return));

    if (x != priv->last_x || y != priv->last_y)
    {
        mt_listener_emit_motion_event (listener, x, y);

        priv->last_x = x;
        priv->last_y = y;

        return TRUE;
    }
    return FALSE;
}

static gboolean
mt_listener_mouse_moving (gpointer data)
{
    if (!mt_listener_mouse_moved (data))
    {
        g_timeout_add (POLL_INTERVAL_IDLE, mt_listener_mouse_idle, data);
        return FALSE;
    }
    return TRUE;
}

static gboolean
mt_listener_mouse_idle (gpointer data)
{
    if (mt_listener_mouse_moved (data))
    {
        g_timeout_add (POLL_INTERVAL_MOVING, mt_listener_mouse_moving, data);
        return FALSE;
    }
    return TRUE;
}

static void
mt_listener_grab_buttons (void)
{
    Display *dpy = mt_listener_display ();

    gdk_error_trap_push ();
    XGrabButton (dpy, AnyButton, AnyModifier,
                 gdk_x11_get_default_root_xwindow (),
                 True, ButtonPressMask | ButtonReleaseMask,
                 GrabModeSync, GrabModeAsync, None, None);
    XSync (dpy, False);
    gdk_error_trap_pop ();
}

static void
mt_listener_ungrab_buttons (void)
{
    Display *dpy = mt_listener_display ();

    gdk_error_trap_push ();
    XUngrabButton (dpy, AnyButton, AnyModifier, XDefaultRootWindow (dpy));
    gdk_error_trap_pop ();
}

static void
mt_listener_forward_button_event (MtListener *listener, XButtonEvent *bev)
{
    MtListenerPrivate *priv = listener->priv;
    guint button_state = bev->state;

    switch (bev->button)
    {
        case 1:
            button_state |= Button1Mask;
            break;
        case 2:
            button_state |= Button2Mask;
            break;
        case 3:
            button_state |= Button3Mask;
            break;
        case 4:
            button_state |= Button4Mask;
            break;
        case 5:
            button_state |= Button5Mask;
            break;
    }

    mt_listener_emit_button_event (listener,
                                   bev->type == ButtonPress ? MT_EVENT_BUTTON_PRESS :
                                                              MT_EVENT_BUTTON_RELEASE,
                                   bev->button,
                                   bev->x_root,
                                   bev->y_root);

    priv->last_x = bev->x_root;
    priv->last_y = bev->y_root;

    if ((button_state & XBUTTON_MASK) != (priv->mask_state & XBUTTON_MASK))
    {
        priv->mask_state = button_state;
    }
}

static GdkFilterReturn
mt_listener_event_filter (GdkXEvent *gdk_xevent,
                          GdkEvent  *gdk_event,
                          gpointer   data)
{
    XEvent *xev = gdk_xevent;

    if (xev->type == ButtonPress || xev->type == ButtonRelease)
    {
        XButtonEvent *bev = (XButtonEvent *) xev;

        mt_listener_forward_button_event (data, bev);
        XAllowEvents (bev->display, ReplayPointer, bev->time);
    }
    return GDK_FILTER_CONTINUE;
}

MtListener *
mt_listener_get_default (void)
{
    static MtListener *listener = NULL;

    if (!listener)
    {
        listener = g_object_new (MT_TYPE_LISTENER, NULL);
        g_object_add_weak_pointer (G_OBJECT (listener), (gpointer *) &listener);
    }
    return listener;
}
