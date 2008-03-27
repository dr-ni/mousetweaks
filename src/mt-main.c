/*
 * Copyright © 2007-2008 Gerd Kohlberger <lowfi@chello.at>
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

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/extensions/Xfixes.h>
#include <cspi/spi.h>

#include "mt-common.h"
#include "mt-service.h"
#include "mt-pidfile.h"
#include "mt-ctw.h"
#include "mt-timer.h"
#include "mt-main.h"

static int fixes_event_base = 0;

static void
mt_cursor_set (GdkCursorType type)
{
    GdkCursor *cursor;

    cursor = gdk_cursor_new (type);
    gdk_window_set_cursor (gdk_get_default_root_window (), cursor);
    gdk_cursor_unref (cursor);
}

static void
dwell_restore_single_click (MTClosure *mt)
{
    if (mt->dwell_mode == DWELL_MODE_CTW)
	mt_ctw_set_clicktype (DWELL_CLICK_TYPE_SINGLE);

    mt_service_set_clicktype (mt->service, DWELL_CLICK_TYPE_SINGLE, NULL);
}

static void
dwell_do_pointer_click (MTClosure *mt, gint x, gint y)
{
    guint clicktype;

    clicktype = mt_service_get_clicktype (mt->service);

    switch (clicktype) {
    case DWELL_CLICK_TYPE_SINGLE:
	SPI_generateMouseEvent (x, y, "b1c");
	break;
    case DWELL_CLICK_TYPE_DOUBLE:
	SPI_generateMouseEvent (x, y, "b1d");
	dwell_restore_single_click (mt);
	break;
    case DWELL_CLICK_TYPE_DRAG:
	SPI_generateMouseEvent (x, y, "b1p");
	mt->dwell_drag_started = TRUE;
	mt_cursor_set (GDK_FLEUR);
	break;
    case DWELL_CLICK_TYPE_RIGHT:
	SPI_generateMouseEvent (x, y, "b3c");
	dwell_restore_single_click (mt);
    default:
	break;
    }
}

static inline gboolean
within_tolerance (MTClosure *mt, gint x, gint y)
{
    gdouble distance;

    distance = sqrt(pow(x - mt->pointer_x, 2.) + pow(y - mt->pointer_y, 2.));

    return distance < mt->threshold;
}

static gboolean
analyze_direction (MTClosure *mt, gint x, gint y)
{
    gint gd, i, dx, dy;

    dx = ABS(x - mt->pointer_x);
    dy = ABS(y - mt->pointer_y);

    if (within_tolerance (mt, x, y))
	return FALSE;

    /* find direction */
    if (x < mt->pointer_x)
	if (y < mt->pointer_y)
	    if (dx < dy)
		gd = DIRECTION_UP;
	    else
		gd = DIRECTION_LEFT;
	else
	    if (dx < dy)
		gd = DIRECTION_DOWN;
	    else
		gd = DIRECTION_LEFT;
    else
	if (y < mt->pointer_y)
	    if (dx < dy)
		gd = DIRECTION_UP;
	    else
		gd = DIRECTION_RIGHT;
	else
	    if (dx < dy)
		gd = DIRECTION_DOWN;
	    else
		gd = DIRECTION_RIGHT;

    /* get click type for direction */
    for (i = 0; i < N_CLICK_TYPES; i++)
	if (mt->dwell_dirs[i] == gd) {
	    mt_service_set_clicktype (mt->service, i, NULL);
	    return TRUE;
	}

    return FALSE;
}

static void
dwell_start_gesture (MTClosure *mt)
{
    if (mt->override_cursor) {
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_CROSS);
	gdk_pointer_grab (gdk_get_default_root_window (),
			  FALSE,
			  GDK_POINTER_MOTION_MASK, 
			  NULL,
			  cursor,
			  GDK_CURRENT_TIME);
	gdk_cursor_unref (cursor);
    }
    else
	mt_cursor_set (GDK_CROSS);

    mt->dwell_gesture_started = TRUE;
    mt_timer_start (mt->dwell_timer);
}

static void
dwell_stop_gesture (MTClosure *mt)
{
    if (mt->override_cursor)
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
    else
	mt_cursor_set (GDK_LEFT_PTR);

    mt->dwell_gesture_started = FALSE;
    mt_timer_stop (mt->dwell_timer);
}

static void
dwell_time_elapsed (MtTimer *timer, gpointer data)
{
    MTClosure *mt = (MTClosure *) data;
    gint x, y;

    gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);

    /* stop active drag */
    if (mt->dwell_drag_started) {
	SPI_generateMouseEvent (x, y, "b1r");

	mt->dwell_drag_started = FALSE;
	mt_cursor_set (GDK_LEFT_PTR);
	dwell_restore_single_click (mt);

	return;
    }

    switch (mt->dwell_mode) {
    case DWELL_MODE_CTW:
	dwell_do_pointer_click (mt, x, y);
	break;
    case DWELL_MODE_GESTURE:
	if (mt->dwell_gesture_started) {
	    dwell_stop_gesture (mt);

	    if (analyze_direction (mt, x, y))
		dwell_do_pointer_click (mt, mt->pointer_x, mt->pointer_y);
	}
	else
	    dwell_start_gesture (mt);
    default:
	break;
    }
}

static void
delay_time_elapsed (MtTimer *timer, gpointer data)
{
    MTClosure *mt = (MTClosure *) data;

    SPI_generateMouseEvent (0, 0, "b1r");
    SPI_generateMouseEvent (mt->pointer_x, mt->pointer_y, "b3c");
}

/* at-spi callbacks */
static void
spi_motion_event (const AccessibleEvent *event, void *data)
{
    MTClosure *mt = (MTClosure *) data;

    if (mt->dwell_enabled)
	if (!within_tolerance (mt, event->detail1, event->detail2))
	    if (!mt->dwell_gesture_started) {
		mt->pointer_x = (gint) event->detail1;
		mt->pointer_y = (gint) event->detail2;
		mt_timer_start (mt->dwell_timer);
	    }

    if (mt_timer_is_running (mt->delay_timer))
	if (!within_tolerance (mt, event->detail1, event->detail2))
	    mt_timer_stop (mt->delay_timer);
}

static void
spi_button_event (const AccessibleEvent *event, void *data)
{
    MTClosure *mt = (MTClosure *) data;

    if (g_str_equal (event->type, "mouse:button:b1p")) {
	if (mt->delay_enabled) {
	    gdk_display_get_pointer (gdk_display_get_default (), NULL,
				     &mt->pointer_x, &mt->pointer_y, NULL);
	    mt_timer_start (mt->delay_timer);
	}

	if (mt->dwell_gesture_started)
	    dwell_stop_gesture (mt);
    }
    else if (g_str_equal (event->type, "mouse:button:b1r"))
	if (mt_timer_is_running (mt->delay_timer))
	    mt_timer_stop (mt->delay_timer);
}

static GdkFilterReturn
cursor_changed (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    XEvent *xev = (XEvent *) xevent;

    if (xev->type == fixes_event_base + XFixesCursorNotify) {
	MTClosure *mt = (MTClosure *) data;
	XFixesCursorImage *ci;

	ci = XFixesGetCursorImage (GDK_DISPLAY());

	if (!mt->dwell_gesture_started)
	    mt->override_cursor = !g_str_equal (ci->name, "left_ptr");

	XFree (ci);
    }

    return GDK_FILTER_CONTINUE;
}

static void
signal_handler (int sig)
{
    SPI_event_quit ();
}

static void
gconf_value_changed (GConfClient *client,
		     const gchar *key,
		     GConfValue  *value,
		     gpointer     data)
{
    MTClosure *mt = (MTClosure *) data;

    if (g_str_equal (key, OPT_THRESHOLD))
	mt->threshold = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_DELAY))
	mt->delay_enabled = gconf_value_get_bool (value);
    else if (g_str_equal (key, OPT_DELAY_T))
	mt_timer_set_target_time (mt->delay_timer,
				  gconf_value_get_float (value));
    else if (g_str_equal (key, OPT_DWELL)) {
	mt->dwell_enabled = gconf_value_get_bool (value);
	mt_ctw_update_sensitivity (mt);
	mt_ctw_update_visibility (mt);
    }
    else if (g_str_equal (key, OPT_DWELL_T))
	mt_timer_set_target_time (mt->dwell_timer,
				  gconf_value_get_float (value));
    else if (g_str_equal (key, OPT_CTW)) {
	mt->dwell_show_ctw = gconf_value_get_bool (value);
	mt_ctw_update_visibility (mt);
    }
    else if (g_str_equal (key, OPT_MODE)) {
	mt->dwell_mode = gconf_value_get_int (value);
	mt_ctw_update_sensitivity (mt);
    }
    else if (g_str_equal (key, OPT_STYLE)) {
	mt->style = gconf_value_get_int (value);
	mt_ctw_update_style (mt->style);
    }
    else if (g_str_equal (key, OPT_G_SINGLE))
	mt->dwell_dirs[DWELL_CLICK_TYPE_SINGLE] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_G_DOUBLE))
	mt->dwell_dirs[DWELL_CLICK_TYPE_DOUBLE] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_G_DRAG))
	mt->dwell_dirs[DWELL_CLICK_TYPE_DRAG] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_G_RIGHT))
	mt->dwell_dirs[DWELL_CLICK_TYPE_RIGHT] = gconf_value_get_int (value);
}

static void
get_gconf_options (MTClosure *mt)
{
    gdouble val;

    mt->threshold = gconf_client_get_int (mt->client, OPT_THRESHOLD, NULL);
    mt->delay_enabled = gconf_client_get_bool (mt->client, OPT_DELAY, NULL);
    mt->dwell_enabled = gconf_client_get_bool (mt->client, OPT_DWELL, NULL);
    mt->dwell_show_ctw = gconf_client_get_bool (mt->client, OPT_CTW, NULL);
    mt->dwell_mode = gconf_client_get_int (mt->client, OPT_MODE, NULL);
    mt->style = gconf_client_get_int (mt->client, OPT_STYLE, NULL);

    val = gconf_client_get_float (mt->client, OPT_DELAY_T, NULL);
    mt_timer_set_target_time (mt->delay_timer, val);
    val = gconf_client_get_float (mt->client, OPT_DWELL_T, NULL);
    mt_timer_set_target_time (mt->dwell_timer, val);

    mt->dwell_dirs[DWELL_CLICK_TYPE_SINGLE] =
	gconf_client_get_int (mt->client, OPT_G_SINGLE, NULL);
    mt->dwell_dirs[DWELL_CLICK_TYPE_DOUBLE] =
	gconf_client_get_int (mt->client, OPT_G_DOUBLE, NULL);
    mt->dwell_dirs[DWELL_CLICK_TYPE_DRAG] =
	gconf_client_get_int (mt->client, OPT_G_DRAG, NULL);
    mt->dwell_dirs[DWELL_CLICK_TYPE_RIGHT] =
	gconf_client_get_int (mt->client, OPT_G_RIGHT, NULL);
}

static MTClosure *
mt_closure_init (void)
{
    MTClosure *mt;

    mt = g_slice_new0 (MTClosure);
    if (!mt)
	return NULL;

    mt->client = gconf_client_get_default ();
    gconf_client_add_dir (mt->client, MT_GCONF_HOME,
			  GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
    g_signal_connect (mt->client, "value_changed",
		      G_CALLBACK (gconf_value_changed), mt);

    mt->delay_timer = mt_timer_new ();
    g_signal_connect (mt->delay_timer, "finished",
		      G_CALLBACK (delay_time_elapsed), mt);

    mt->dwell_timer = mt_timer_new ();
    g_signal_connect (mt->dwell_timer, "finished",
		      G_CALLBACK (dwell_time_elapsed), mt);

    mt->service = mt_service_get_default ();
    mt_service_set_clicktype (mt->service, DWELL_CLICK_TYPE_SINGLE, NULL);

    return mt;
}

static void
mt_closure_free (MTClosure *mt)
{
    g_object_unref (mt->delay_timer);
    g_object_unref (mt->dwell_timer);
    g_object_unref (mt->service);
    g_object_unref (mt->client);

    g_slice_free (MTClosure, mt);
}

int
main (int argc, char **argv)
{
    MTClosure *mt;
    AccessibleEventListener *bl, *ml;
    int fixes_error_base;
    pid_t pid;
    gboolean shutdown = FALSE, ctw = FALSE;
    gchar *mode = NULL, *enable = NULL;
    gint pos_x = -1, pos_y = -1;
    GOptionContext *context;
    GOptionEntry entries[] = {
	{"enable", 'e', 0, G_OPTION_ARG_STRING, &enable,
	    _("Enable mousetweaks feature"), "(dwell|delay)"},
	{"dwell-mode", 'm', 0, G_OPTION_ARG_STRING, &mode,
	    _("Use dwell mode"), "(window|gesture)"},
	{"show-ctw", 'c', 0, G_OPTION_ARG_NONE, &ctw,
	    _("Show click type window"), 0},
	{"ctw-x", 'x', 0, G_OPTION_ARG_INT, &pos_x,
	    _("Window x position"), 0},
	{"ctw-y", 'y', 0, G_OPTION_ARG_INT, &pos_y,
	    _("Window y position"), 0},
	{"shutdown", 's', 0, G_OPTION_ARG_NONE, &shutdown,
	    _("Shut down mousetweaks"), 0},
	{NULL}
    };

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    context = g_option_context_new (_("- GNOME mousetweaks daemon"));
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (FALSE));
    g_option_context_parse (context, &argc, &argv, NULL);
    g_option_context_free (context);

    if (shutdown) {
	int ret;

	if ((ret = mt_pidfile_kill_wait (SIGINT, 5)) < 0)
	    g_print ("Shutdown failed or nothing to shut down.\n");
	else
	    g_print ("Shutdown successful.\n");

	return ret < 0 ? 1 : 0;
    }

    if ((pid = mt_pidfile_is_running()) >= 0) {
	g_print ("Daemon is already running. (PID %u)\n", pid);
	return 1;
    }

    g_print ("Starting daemon.\n");

    if ((pid = fork ()) < 0) {
	g_print ("Fork failed.\n");
	return 1;
    }
    else if (pid) {
	return 0;
    }
    else {
	if (mt_pidfile_create () < 0)
	    goto FINISH;

	signal (SIGINT, signal_handler);
	signal (SIGTERM, signal_handler);
	signal (SIGQUIT, signal_handler);
	signal (SIGHUP, signal_handler);

	gtk_init (&argc, &argv);

	mt = mt_closure_init ();
	if (!mt)
	    goto FINISH;

	if (SPI_init () != 0) {
	    mt_show_dialog (_("Assistive Technologies not enabled"),
			    _("Mousetweaks requires Assistive Technologies."),
			    GTK_MESSAGE_ERROR);

	    /* make sure we leave in a clean state */
	    gconf_client_set_bool (mt->client, OPT_DELAY, FALSE, NULL);
	    gconf_client_set_bool (mt->client, OPT_DWELL, FALSE, NULL);

	    mt_closure_free (mt);
	    goto FINISH;
	}

	/* listen for cursor changes */
	if (XFixesQueryExtension (GDK_DISPLAY(),
				  &fixes_event_base,
				  &fixes_error_base)) {
	    XFixesSelectCursorInput (GDK_DISPLAY(),
				     GDK_ROOT_WINDOW(),
				     XFixesDisplayCursorNotifyMask);
	    gdk_window_add_filter (gdk_get_default_root_window (),
				   cursor_changed, mt);
	}

	/* add at-spi listeners */
	ml = SPI_createAccessibleEventListener (spi_motion_event, (void *) mt);
	SPI_registerGlobalEventListener (ml, "mouse:abs");
	bl = SPI_createAccessibleEventListener (spi_button_event, (void *) mt);
	SPI_registerGlobalEventListener (bl, "mouse:button:1p");
	SPI_registerGlobalEventListener (bl, "mouse:button:1r");

	/* command-line options */
	if (enable) {
	    if (g_str_equal (enable, "dwell")) {
		gconf_client_set_bool (mt->client, OPT_DELAY, FALSE, NULL);
		gconf_client_set_bool (mt->client, OPT_DWELL, TRUE, NULL);
	    }
	    if (g_str_equal (enable, "delay")) {
		gconf_client_set_bool (mt->client, OPT_DWELL, FALSE, NULL);
		gconf_client_set_bool (mt->client, OPT_DELAY, TRUE, NULL);
	    }
	}

	if (ctw)
	    gconf_client_set_bool (mt->client, OPT_CTW, TRUE, NULL);

	if (mode) {
	    if (g_str_equal (mode, "gesture"))
		gconf_client_set_int (mt->client, OPT_MODE,
				      DWELL_MODE_GESTURE, NULL);
	    else if (g_str_equal (mode, "window"))
		gconf_client_set_int (mt->client, OPT_MODE,
				      DWELL_MODE_CTW, NULL);
	}

	get_gconf_options (mt);

	if (!mt_ctw_init (mt, pos_x, pos_y))
	    goto CLEANUP;

	SPI_event_main ();
    }

CLEANUP:
    SPI_deregisterGlobalEventListenerAll (bl);
    AccessibleDeviceListener_unref (bl);
    SPI_deregisterGlobalEventListenerAll (ml);
    AccessibleEventListener_unref (ml);
    SPI_exit ();
    mt_closure_free (mt);

FINISH:
    mt_pidfile_remove ();

    return 0;
}
