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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/extensions/XTest.h>

#include "mt-common.h"
#include "mt-settings.h"
#include "mt-service.h"
#include "mt-pidfile.h"
#include "mt-ctw.h"
#include "mt-timer.h"
#include "mt-cursor-manager.h"
#include "mt-cursor.h"
#include "mt-listener.h"
#include "mt-sig-handler.h"

enum
{
    PRESS = 0,
    RELEASE,
    CLICK,
    DOUBLE_CLICK
};

typedef struct _MtCliArgs
{
    gdouble  ssc_time;
    gdouble  dwell_time;
    gchar   *mode;
    gchar   *geometry;
    gint     threshold;
    gboolean ssc_enabled;
    gboolean dwell_enabled;
    gboolean shutdown;
    gboolean daemonize;
    gboolean ctw;
    gboolean login;
} MtCliArgs;

typedef struct _MtData
{
    MtService   *service;
    MtTimer     *ssc_timer;
    MtTimer     *dwell_timer;
    gint         direction;
    gint         pointer_x;
    gint         pointer_y;

    /* state flags */
    guint        dwell_drag_started    : 1;
    guint        dwell_gesture_started : 1;
    guint        override_cursor       : 1;
    guint        ssc_finished          : 1;
} MtData;

static void
mt_main_generate_motion_event (GdkScreen *screen, gint x, gint y)
{
    GdkDevice *cp;

    cp = mt_common_get_client_pointer ();
    if (cp)
        gdk_device_warp (cp, screen, x, y);
}

static void
mt_main_generate_button_event (MtData *mt,
                               guint   button,
                               gint    type,
                               gulong  delay)
{
    Display *dpy;

    dpy = mt_common_get_xdisplay ();
    mt_common_xtrap_push ();
    switch (type)
    {
        case PRESS:
            XTestFakeButtonEvent (dpy, button, True, delay);
            break;
        case RELEASE:
            XTestFakeButtonEvent (dpy, button, False, delay);
            break;
        case CLICK:
            XTestFakeButtonEvent (dpy, button, True, CurrentTime);
            XTestFakeButtonEvent (dpy, button, False, delay);
            break;
        case DOUBLE_CLICK:
            XTestFakeButtonEvent (dpy, button, True, CurrentTime);
            XTestFakeButtonEvent (dpy, button, False, delay);
            XTestFakeButtonEvent (dpy, button, True, delay);
            XTestFakeButtonEvent (dpy, button, False, delay);
            break;
        default:
            g_warning ("Unknown button sequence.");
            break;
    }
    mt_common_xtrap_pop ();
}

static void
mt_main_set_cursor (MtData *mt, GdkCursorType type)
{
    GdkDisplay *gdk_dpy;
    GdkScreen *screen;
    GdkCursor *cursor;
    gint n_screens, i;

    gdk_dpy = gdk_display_get_default ();
    n_screens = gdk_display_get_n_screens (gdk_dpy);

    cursor = gdk_cursor_new (type);
    for (i = 0; i < n_screens; ++i)
    {
        screen = gdk_display_get_screen (gdk_dpy, i);
        gdk_window_set_cursor (gdk_screen_get_root_window (screen), cursor);
    }
    g_object_unref (cursor);
}

static void
mt_main_do_dwell_click (MtData *mt)
{
    MtDwellClickType click_type;
    MtSettings *ms;

    ms = mt_settings_get_default ();
    click_type = mt_service_get_click_type (mt->service);

    if (ms->dwell_mode == G_DESKTOP_MOUSE_DWELL_MODE_GESTURE &&
        !mt->dwell_drag_started)
    {
        mt_main_generate_motion_event (mt_common_get_screen (),
                                       mt->pointer_x,
                                       mt->pointer_y);
    }

    switch (click_type)
    {
        case MT_DWELL_CLICK_TYPE_SINGLE:
            mt_main_generate_button_event (mt, 1, CLICK, 80);
            break;
        case MT_DWELL_CLICK_TYPE_DOUBLE:
            mt_main_generate_button_event (mt, 1, DOUBLE_CLICK, 40);
            mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_SINGLE);
            break;
        case MT_DWELL_CLICK_TYPE_DRAG:
            if (!mt->dwell_drag_started)
            {
                mt_main_generate_button_event (mt, 1, PRESS, CurrentTime);
                mt_main_set_cursor (mt, GDK_FLEUR);
                mt->dwell_drag_started = TRUE;
            }
            else
            {
                mt_main_generate_button_event (mt, 1, RELEASE, CurrentTime);
                mt_main_set_cursor (mt, GDK_LEFT_PTR);
                mt->dwell_drag_started = FALSE;
                mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_SINGLE);
            }
            break;
        case MT_DWELL_CLICK_TYPE_RIGHT:
            mt_main_generate_button_event (mt, 3, CLICK, 80);
            mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_SINGLE);
            break;
        case MT_DWELL_CLICK_TYPE_MIDDLE:
            mt_main_generate_button_event (mt, 2, CLICK, 80);
            mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_SINGLE);
            break;
        default:
            g_warning ("Unknown click-type.");
            break;
    }
}

static inline gboolean
below_threshold (MtData *mt, gint x, gint y)
{
    MtSettings *ms;
    gint dx, dy;

    ms = mt_settings_get_default ();
    dx = x - mt->pointer_x;
    dy = y - mt->pointer_y;

    return (dx * dx + dy * dy) < (ms->dwell_threshold * ms->dwell_threshold);
}

static GDesktopMouseDwellDirection
mt_main_get_direction (MtData *mt, gint x, gint y)
{
    gint dx, dy;

    dx = ABS (mt->pointer_x - x);
    dy = ABS (mt->pointer_y - y);

    if (mt->pointer_x < x)
    {
        if (dx > dy)
            return G_DESKTOP_MOUSE_DWELL_DIRECTION_LEFT;
    }
    else
    {
        if (dx > dy)
            return G_DESKTOP_MOUSE_DWELL_DIRECTION_RIGHT;
    }

    return mt->pointer_y < y ? G_DESKTOP_MOUSE_DWELL_DIRECTION_UP :
                               G_DESKTOP_MOUSE_DWELL_DIRECTION_DOWN;
}

static gboolean
mt_main_analyze_gesture (MtData *mt)
{
    MtSettings *ms;
    GDesktopMouseDwellDirection direction;
    GdkDevice *cp;
    gint x, y;

    if (mt_service_get_click_type (mt->service) == MT_DWELL_CLICK_TYPE_DRAG)
        return TRUE;

    cp = mt_common_get_client_pointer ();
    if (!cp)
        return FALSE;

    gdk_device_get_position (cp, NULL, &x, &y);

    if (below_threshold (mt, x, y))
        return FALSE;

    direction = mt_main_get_direction (mt, x, y);
    ms = mt_settings_get_default ();

    if (direction == ms->dwell_gesture_single)
    {
        mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_SINGLE);
    }
    else if (direction == ms->dwell_gesture_double)
    {
        mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_DOUBLE);
    }
    else if (direction == ms->dwell_gesture_drag)
    {
        mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_DRAG);
    }
    else if (direction == ms->dwell_gesture_secondary)
    {
        mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_RIGHT);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

static void
dwell_start_gesture (MtData *mt)
{
    GdkDevice *cp;
    GdkCursor *cursor;
    GdkWindow *root;

    if (mt->override_cursor)
    {
        cp = mt_common_get_client_pointer ();
        if (cp)
        {
            cursor = gdk_cursor_new (GDK_CROSS);
            root = gdk_screen_get_root_window (mt_common_get_screen ());
            gdk_device_grab (cp, root,
                             GDK_OWNERSHIP_NONE, FALSE,
                             GDK_POINTER_MOTION_MASK,
                             cursor,
                             gtk_get_current_event_time ());
            g_object_unref (cursor);
        }
    }
    else
    {
        mt_main_set_cursor (mt, GDK_CROSS);
    }

    mt->dwell_gesture_started = TRUE;
    mt_timer_start (mt->dwell_timer);
}

static void
dwell_stop_gesture (MtData *mt)
{
    GdkDevice *cp;

    if (mt->override_cursor)
    {
        cp = mt_common_get_client_pointer ();
        if (cp)
            gdk_device_ungrab (cp, gtk_get_current_event_time ());
    }
    else
    {
        mt_main_set_cursor (mt, GDK_LEFT_PTR);
    }

    mt->dwell_gesture_started = FALSE;
    mt_timer_stop (mt->dwell_timer);
}

static void
dwell_timer_finished (MtTimer *timer, MtData *mt)
{
    MtSettings *ms;

    ms = mt_settings_get_default ();
    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

    if (ms->dwell_mode == G_DESKTOP_MOUSE_DWELL_MODE_WINDOW)
    {
        mt_main_do_dwell_click (mt);
    }
    else
    {
        if (mt->dwell_gesture_started)
        {
            dwell_stop_gesture (mt);

            if (mt_main_analyze_gesture (mt))
                mt_main_do_dwell_click (mt);
        }
        else if (mt->dwell_drag_started)
        {
            /* if a drag action is in progress stop it */
            mt_main_do_dwell_click (mt);
        }
        else
        {
            dwell_start_gesture (mt);
        }
    }
}

static void
ssc_timer_finished (MtTimer *timer, MtData *mt)
{
    mt->ssc_finished = TRUE;
}

static void
mt_main_do_secondary_click (MtData *mt)
{
    mt->ssc_finished = FALSE;
    mt_main_generate_button_event (mt, 3, CLICK, CurrentTime);
}

static void
mt_dwell_click_cancel (MtData *mt)
{
    if (mt->dwell_gesture_started)
    {
        dwell_stop_gesture (mt);
        return;
    }

    mt_timer_stop (mt->dwell_timer);
    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

    if (mt->dwell_drag_started)
    {
        mt_main_set_cursor (mt, GDK_LEFT_PTR);
        mt->dwell_drag_started = FALSE;
    }

    mt_service_set_click_type (mt->service, MT_DWELL_CLICK_TYPE_SINGLE);
}

static void
global_motion_event (MtListener *listener,
                     MtEvent    *event,
                     MtData     *mt)
{
    MtSettings *ms;

    ms = mt_settings_get_default ();

    if (ms->ssc_enabled && !below_threshold (mt, event->x, event->y))
    {
        mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

        if (mt_timer_is_running (mt->ssc_timer))
            mt_timer_stop (mt->ssc_timer);

        if (mt->ssc_finished)
            mt->ssc_finished = FALSE;
    }

    if (ms->dwell_enabled && !below_threshold (mt, event->x, event->y) &&
        !mt->dwell_gesture_started)
    {
        mt->pointer_x = event->x;
        mt->pointer_y = event->y;
        mt_timer_start (mt->dwell_timer);
    }
}

static void
global_button_event (MtListener *listener,
                     MtEvent    *event,
                     MtData     *mt)
{
    MtSettings *ms;

    ms = mt_settings_get_default ();

    if (ms->ssc_enabled && event->button == 1)
    {
        if (event->type == MT_EVENT_BUTTON_PRESS)
        {
            mt->pointer_x = event->x;
            mt->pointer_y = event->y;
            mt_timer_start (mt->ssc_timer);
        }
        else
        {
            mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

            if (mt->ssc_finished)
                mt_main_do_secondary_click (mt);
            else
                mt_timer_stop (mt->ssc_timer);
        }
    }
    /*
     * cancel a dwell-click in progress if a physical button
     * is pressed - useful for mixed use-cases and testing
     */
    if ((event->type == MT_EVENT_BUTTON_PRESS && mt_timer_is_running (mt->dwell_timer)) ||
        (event->type == MT_EVENT_BUTTON_RELEASE && mt->dwell_drag_started))
    {
        mt_dwell_click_cancel (mt);
    }
}

static void
cursor_overlay_time (guchar *image,
                     gint    width,
                     gint    height,
                     gdouble target,
                     gdouble elapsed)
{
    GdkColor color;
    GtkStyle *style;
    cairo_surface_t *surface;
    cairo_t *cr;

    style = gtk_widget_get_style (mt_ctw_get_window ());
    color = style->bg[GTK_STATE_SELECTED];

    surface = cairo_image_surface_create_for_data (image,
                                                   CAIRO_FORMAT_ARGB32,
                                                   width, height,
                                                   width * 4);
    cr = cairo_create (surface);
    cairo_set_operator (cr, CAIRO_OPERATOR_ATOP);
    cairo_rectangle (cr, 0, 0, width, (height * elapsed) / target);
    cairo_set_source_rgba (cr,
                           color.red   / 65535.,
                           color.green / 65535.,
                           color.blue  / 65535.,
                           0.60);
    cairo_fill (cr);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);
}

static void
mt_main_timer_tick (MtTimer *timer, gdouble elapsed, gpointer data)
{
    MtCursorManager *manager;
    MtCursor *current_cursor, *new_cursor;

    manager = mt_cursor_manager_get_default ();
    current_cursor = mt_cursor_manager_get_current_cursor (manager);

    if (current_cursor)
    {
        const gchar *name;
        gushort width, height, xhot, yhot;
        gdouble target;
        guchar *image;

        /* get cursor info */
        name = mt_cursor_get_name (current_cursor);
        image = mt_cursor_get_image_copy (current_cursor);
        mt_cursor_get_dimension (current_cursor, &width, &height);
        mt_cursor_get_hotspot (current_cursor, &xhot, &yhot);

        g_object_unref (current_cursor);

        target = mt_timer_get_target (timer);

        /* paint overlay */
        cursor_overlay_time (image, width, height, target, elapsed);

        /* create and set new cursor */
        new_cursor = mt_cursor_new (name, image, width, height, xhot, yhot);
        if (new_cursor)
        {
            mt_cursor_manager_set_cursor (manager, new_cursor);
            g_object_unref (new_cursor);
        }
    }
}

static void
mt_main_cursor_changed (MtCursorManager *manager,
                        const gchar     *name,
                        MtData          *mt)
{
    if (!mt->dwell_gesture_started)
    {
        /* Remove me, I'm weird */
        mt->override_cursor = !g_str_equal (name, "left_ptr");
    }
}

static void
signal_handler (int signal_id)
{
    gtk_main_quit ();
}

static void
mt_main_sig_handler (MtSigHandler *sigh,
                     gint          signal_id,
                     gpointer      data)
{
    mt_ctw_save_geometry ();
    signal_handler (signal_id);
}

static MtData *
mt_data_init (void)
{
    Display *dpy;
    MtData *mt;
    gint nil;

    mt = g_slice_new0 (MtData);
    dpy = mt_common_get_xdisplay ();

    if (!XTestQueryExtension (dpy, &nil, &nil, &nil, &nil))
    {
        g_slice_free (MtData, mt);
        g_critical ("No XTest extension found. Aborting.");
        return NULL;
    }

    /* continue sending event requests inspite of other grabs */
    XTestGrabControl (dpy, True);

    mt->ssc_timer = mt_timer_new ();
    g_signal_connect (mt->ssc_timer, "finished",
                      G_CALLBACK (ssc_timer_finished), mt);
    g_signal_connect (mt->ssc_timer, "tick",
                      G_CALLBACK (mt_main_timer_tick), mt);

    mt->dwell_timer = mt_timer_new ();
    g_signal_connect (mt->dwell_timer, "finished",
                      G_CALLBACK (dwell_timer_finished), mt);
    g_signal_connect (mt->dwell_timer, "tick",
                      G_CALLBACK (mt_main_timer_tick), mt);

    mt->service = mt_service_get_default ();

    return mt;
}

static void
mt_data_free (MtData *mt)
{
    g_object_unref (mt->ssc_timer);
    g_object_unref (mt->dwell_timer);
    g_object_unref (mt->service);
    g_slice_free (MtData, mt);
}

static MtCliArgs
mt_parse_options (int *argc, char ***argv)
{
    MtCliArgs ca;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] =
    {
        {"dwell", 0, 0, G_OPTION_ARG_NONE, &ca.dwell_enabled,
            N_("Enable dwell click"), NULL},
        {"ssc", 0, 0, G_OPTION_ARG_NONE, &ca.ssc_enabled,
            N_("Enable simulated secondary click"), NULL},
        {"dwell-time", 0, 0, G_OPTION_ARG_DOUBLE, &ca.dwell_time,
            N_("Time to wait before a dwell click"), "[0.2-3.0]"},
        {"ssc-time", 0, 0, G_OPTION_ARG_DOUBLE, &ca.ssc_time,
            N_("Time to wait before a simulated secondary click"), "[0.5-3.0]"},
        {"dwell-mode", 'm', 0, G_OPTION_ARG_STRING, &ca.mode,
            N_("Set the active dwell mode"), "[window|gesture]"},
        {"hide-ctw", 'c', 0, G_OPTION_ARG_NONE, &ca.ctw,
            N_("Hide the click-type window"), NULL},
        {"threshold", 't', 0, G_OPTION_ARG_INT, &ca.threshold,
            N_("Ignore small pointer movements"), "[0-30]"},
        {"geometry", 'g', 0, G_OPTION_ARG_STRING, &ca.geometry,
            N_("Click-type window geometry"), "WIDTHxHEIGHT+X+Y"},
        {"shutdown", 's', 0, G_OPTION_ARG_NONE, &ca.shutdown,
            N_("Shut down mousetweaks"), NULL},
        {"daemonize", 0, 0, G_OPTION_ARG_NONE, &ca.daemonize,
            N_("Start mousetweaks as a daemon"), NULL},
        {"login", 0, 0, G_OPTION_ARG_NONE, &ca.login,
            N_("Start mousetweaks in login mode"), NULL},
        { NULL }
    };

    /* init cli arguments */
    ca.ssc_time      = -1.;
    ca.dwell_time    = -1.;
    ca.mode          = NULL;
    ca.geometry      = NULL;
    ca.threshold     = -1;
    ca.ssc_enabled   = FALSE;
    ca.dwell_enabled = FALSE;
    ca.shutdown      = FALSE;
    ca.daemonize     = FALSE;
    ca.ctw           = FALSE;
    ca.login         = FALSE;

    /* parse */
    context = g_option_context_new (_("- GNOME mouse accessibility daemon"));
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, argc, argv, &error))
    {
        g_print ("%s\n", error->message);
        g_error_free (error);
        g_option_context_free (context);
        exit (1);
    }
    g_option_context_free (context);

    return ca;
}

static void
mt_main (int argc, char **argv, MtCliArgs cli_args)
{
    MtData *mt;
    MtCursorManager *manager;
    MtSettings *ms;
    MtListener *listener;
    MtSigHandler *sigh;

    if (mt_pidfile_create () < 0)
    {
        g_warning ("Couldn't create PID file.");
        return;
    }

    gtk_init (&argc, &argv);

    sigh = mt_sig_handler_get_default ();
    if (mt_sig_handler_setup_pipe (sigh))
    {
        mt_sig_handler_catch (sigh, SIGINT);
        mt_sig_handler_catch (sigh, SIGTERM);
        mt_sig_handler_catch (sigh, SIGQUIT);
        mt_sig_handler_catch (sigh, SIGHUP);

        g_signal_connect (sigh, "signal",
                          G_CALLBACK (mt_main_sig_handler), NULL);
    }
    else
    {
        g_warning ("Couldn't create pipe for signal handling. Using fallback.");

        signal (SIGINT, signal_handler);
        signal (SIGTERM, signal_handler);
        signal (SIGQUIT, signal_handler);
        signal (SIGHUP, signal_handler);
    }

    mt = mt_data_init ();
    if (!mt)
        goto FINISH;

    /* load settings */
    ms = mt_settings_get_default ();

    /* bind timers */
    g_settings_bind (ms->a11y_settings, KEY_SSC_TIME,
                     mt->ssc_timer, "target-time",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);

    g_settings_bind (ms->a11y_settings, KEY_DWELL_TIME,
                     mt->dwell_timer, "target-time",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);

    /* override with CLI arguments */
    if (cli_args.dwell_enabled)
        ms->dwell_enabled = cli_args.dwell_enabled;
    if (cli_args.ssc_enabled)
        ms->ssc_enabled = cli_args.ssc_enabled;
    if (cli_args.dwell_time >= .1 && cli_args.dwell_time <= 3.)
        mt_timer_set_target (mt->dwell_timer, cli_args.dwell_time);
    if (cli_args.ssc_time >= .1 && cli_args.ssc_time <= 3.)
        mt_timer_set_target (mt->ssc_timer, cli_args.ssc_time);
    if (cli_args.threshold >= 0 && cli_args.threshold <= 30)
        ms->dwell_threshold = cli_args.threshold;
    if (cli_args.ctw)
        ms->ctw_visible = !cli_args.ctw;
    if (cli_args.mode)
    {
        if (g_str_equal (cli_args.mode, "gesture"))
            ms->dwell_mode = G_DESKTOP_MOUSE_DWELL_MODE_GESTURE;
        else if (g_str_equal (cli_args.mode, "window"))
            ms->dwell_mode = G_DESKTOP_MOUSE_DWELL_MODE_WINDOW;

        g_free (cli_args.mode);
    }
    if (cli_args.geometry)
    {
        g_free (ms->ctw_geometry);
        ms->ctw_geometry = cli_args.geometry;
    }

    /* init click-type window */
    if (!mt_ctw_init ())
        goto CLEANUP;

    /* init cursor animation */
    manager = mt_cursor_manager_get_default ();
    g_signal_connect (manager, "cursor_changed",
                      G_CALLBACK (mt_main_cursor_changed), mt);

    /* init mouse listener */
    listener = mt_listener_get_default ();

    if (ms->dwell_enabled)
        mt_listener_grab_mouse_wheel (listener);

    g_signal_connect (listener, "motion_event",
                      G_CALLBACK (global_motion_event), mt);
    g_signal_connect (listener, "button_event",
                      G_CALLBACK (global_button_event), mt);

    gtk_main ();

    mt_ctw_fini ();
    mt_cursor_manager_restore_all (manager);
    g_object_unref (manager);
    g_object_unref (listener);
    g_object_unref (sigh);

CLEANUP:
    g_object_unref (ms);
    mt_data_free (mt);

FINISH:
    mt_pidfile_remove ();
}

int
main (int argc, char **argv)
{
    MtCliArgs cli_args;
    pid_t pid;

    bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    setlocale (LC_ALL, "");

    g_set_application_name ("Mousetweaks");
    cli_args = mt_parse_options (&argc, &argv);

    if (cli_args.shutdown)
    {
        int ret;

        if ((ret = mt_pidfile_kill_wait (SIGINT, 5)) < 0)
            g_print ("Shutdown failed or nothing to shut down.\n");
        else
            g_print ("Shutdown successful.\n");

        return ret < 0 ? 1 : 0;
    }

    if ((pid = mt_pidfile_is_running ()) >= 0)
    {
        g_print ("Mousetweaks is already running. (PID %u)\n", pid);
        return 1;
    }

    if (cli_args.daemonize)
    {
        g_print ("Starting daemon.\n");
        if ((pid = fork ()) < 0)
        {
            g_error ("fork() failed.");
            return 1;
        }
        else if (pid)
        {
            return 0;
        }
    }
    mt_main (argc, argv, cli_args);

    return 0;
}
