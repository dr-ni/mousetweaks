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
#include <cspi/spi.h>

#include <libgnome/gnome-program.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-client.h>

#include "mt-common.h"
#include "mt-service.h"
#include "mt-pidfile.h"
#include "mt-ctw.h"
#include "mt-timer.h"
#include "mt-cursor-manager.h"
#include "mt-cursor.h"
#include "mt-main.h"

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
	if (!mt->dwell_drag_started) {
	    SPI_generateMouseEvent (x, y, "b1p");
	    mt->dwell_drag_started = TRUE;
	    mt_cursor_set (GDK_FLEUR);
	}
	else {
	    SPI_generateMouseEvent (x, y, "b1r");
	    mt->dwell_drag_started = FALSE;
	    mt_cursor_set (GDK_LEFT_PTR);
	    dwell_restore_single_click (mt);
	}
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
draw_line (gint x1, gint y1, gint x2, gint y2)
{
    GdkScreen *screen;
    GdkWindow *root;
    GdkGC *gc;

    screen = gdk_display_get_default_screen (gdk_display_get_default ());
    root = gdk_screen_get_root_window (screen);

    gc = gdk_gc_new (root);
    gdk_gc_set_subwindow (gc, GDK_INCLUDE_INFERIORS);
    gdk_gc_set_function (gc, GDK_INVERT);
    gdk_gc_set_line_attributes (gc, 1,
				GDK_LINE_SOLID,
				GDK_CAP_ROUND,
				GDK_JOIN_ROUND);
    gdk_draw_arc (root, gc, TRUE,
		  x1 - 4, y1 - 4, 8, 8, 0, 23040);
    gdk_draw_line (root, gc, x1, y1, x2, y2);
    g_object_unref (gc);
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

    if (mt->x_old > -1 && mt->y_old > -1) {
	draw_line (mt->pointer_x, mt->pointer_y, mt->x_old, mt->y_old);
	mt->x_old = -1;
	mt->y_old = -1;
    }

    mt->dwell_gesture_started = FALSE;
    mt_timer_stop (mt->dwell_timer);
}

static void
dwell_timer_finished (MtTimer *timer, gpointer data)
{
    MTClosure *mt = data;
    gint x, y;

    gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

    if (mt->dwell_mode == DWELL_MODE_CTW)
	dwell_do_pointer_click (mt, x, y);
    else if (mt->dwell_mode == DWELL_MODE_GESTURE) {
	if (mt->dwell_gesture_started) {
	    dwell_stop_gesture (mt);
	    if (analyze_direction (mt, x, y))
		dwell_do_pointer_click (mt, mt->pointer_x, mt->pointer_y);
	}
	else
	    dwell_start_gesture (mt);
    }
}

static gboolean
right_click_timeout (gpointer data)
{
    MTClosure *mt = (MTClosure *) data;

    SPI_generateMouseEvent (mt->pointer_x, mt->pointer_y, "b3c");

    return FALSE;
}

static void
delay_timer_finished (MtTimer *timer, gpointer data)
{
    MTClosure *mt = (MTClosure *) data;

    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

    SPI_generateMouseEvent (0, 0, "b1r");
    SPI_generateMouseEvent (mt->pointer_x, mt->pointer_y, "abs");

    g_timeout_add (100, right_click_timeout, data);
}

/* at-spi callbacks */
static void
spi_motion_event (const AccessibleEvent *event, void *data)
{
    MTClosure *mt = (MTClosure *) data;

    if (mt->dwell_enabled) {
	if (!within_tolerance (mt, event->detail1, event->detail2) &&
	    !mt->dwell_gesture_started) {
	    mt->pointer_x = (gint) event->detail1;
	    mt->pointer_y = (gint) event->detail2;
	    mt_timer_start (mt->dwell_timer);
	}
	if (mt->dwell_gesture_started) {
	    if (mt->x_old > -1 && mt->y_old > -1)
		draw_line (mt->pointer_x, mt->pointer_y, mt->x_old, mt->y_old);

	    draw_line (mt->pointer_x, mt->pointer_y,
		       event->detail1, event->detail2);

	    mt->x_old = event->detail1;
	    mt->y_old = event->detail2;
	}
    }
    if (mt_timer_is_running (mt->delay_timer)) {
	if (!within_tolerance (mt, event->detail1, event->detail2)) {
	    mt_timer_stop (mt->delay_timer);
	    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());
	}
    }
}

static void
spi_button_event (const AccessibleEvent *event, void *data)
{
    MTClosure *mt = (MTClosure *) data;

    if (g_str_equal (event->type, "mouse:button:1p")) {
	if (mt->delay_enabled) {
	    mt->pointer_x = (gint) event->detail1;
	    mt->pointer_y = (gint) event->detail2;
	    mt_timer_start (mt->delay_timer);
	}
	if (mt->dwell_gesture_started)
	    dwell_stop_gesture (mt);
    }
    else if (g_str_equal (event->type, "mouse:button:1r")) {
	if (mt->delay_enabled) {
	    mt_timer_stop (mt->delay_timer);
	    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());
	}
    }
}

static gboolean
cursor_overlay_time (guchar  *image,
		     gint     width,
		     gint     height,
		     MtTimer *timer,
		     gdouble  time)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    gdouble target;

    surface = cairo_image_surface_create_for_data (image,
						   CAIRO_FORMAT_ARGB32,
						   width, height,
						   width * 4);
    if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
	return FALSE;

    cr = cairo_create (surface);
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
	cairo_surface_destroy (surface);
	return FALSE;
    }

    target = mt_timer_get_target (timer);
    cairo_set_operator (cr, CAIRO_OPERATOR_ATOP);
    cairo_rectangle (cr, 0, 0, width, height / (target / time));
    cairo_set_source_rgba (cr, 0., 0., 0., 0.75);
    cairo_fill (cr);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return TRUE;
}

void
mt_update_cursor (MtCursor *cursor, MtTimer *timer, gdouble time)
{
    guchar *image;
    gushort width, height;

    image = mt_cursor_get_image_copy (cursor);
    if (image == NULL)
	return;

    mt_cursor_get_dimension (cursor, &width, &height);

    if (cursor_overlay_time (image, width, height, timer, time)) {
	MtCursorManager *manager;
	MtCursor *new_cursor;
	const gchar *name;
	gushort xhot, yhot;

	name = mt_cursor_get_name (cursor);
	mt_cursor_get_hotspot (cursor, &xhot, &yhot);
	new_cursor = mt_cursor_new (name, image, width, height, xhot, yhot);
	manager = mt_cursor_manager_get_default ();
	mt_cursor_manager_set_cursor (manager, new_cursor);
	g_object_unref (new_cursor);
    }

    g_free (image);
}

static void
delay_timer_tick (MtTimer *timer, gdouble time, gpointer data)
{
    MTClosure *mt = (MTClosure *) data;

    if (mt->animate_cursor && mt->cursor != NULL)
	mt_update_cursor (mt->cursor, timer, time);
}

static void
dwell_timer_tick (MtTimer *timer, gdouble time, gpointer data)
{
    MTClosure *mt = (MTClosure *) data;

    if (mt->animate_cursor && mt->cursor != NULL)
	mt_update_cursor (mt->cursor, timer, time);
}

static void
cursor_cache_cleared (MtCursorManager *manager,
		      gpointer         data)
{
    MTClosure *mt = (MTClosure *) data;

    mt->cursor = mt_cursor_manager_current_cursor (manager);
}

static void
cursor_changed (MtCursorManager *manager,
		const gchar     *name,
		gpointer         data)
{
    MTClosure *mt = (MTClosure *) data;

    if (!mt->dwell_gesture_started)
	mt->override_cursor = !g_str_equal (name, "left_ptr");

    mt->cursor = mt_cursor_manager_lookup_cursor (manager, name);
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

    if (g_str_equal (key, OPT_THRESHOLD) && value->type == GCONF_VALUE_INT)
	mt->threshold = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_DELAY) && value->type == GCONF_VALUE_BOOL)
	mt->delay_enabled = gconf_value_get_bool (value);
    else if (g_str_equal (key, OPT_DELAY_T) && value->type == GCONF_VALUE_FLOAT)
	mt_timer_set_target (mt->delay_timer, gconf_value_get_float (value));
    else if (g_str_equal (key, OPT_DWELL) && value->type == GCONF_VALUE_BOOL) {
	mt->dwell_enabled = gconf_value_get_bool (value);
	mt_ctw_update_sensitivity (mt);
	mt_ctw_update_visibility (mt);
    }
    else if (g_str_equal (key, OPT_DWELL_T) && value->type == GCONF_VALUE_FLOAT)
	mt_timer_set_target (mt->dwell_timer, gconf_value_get_float (value));
    else if (g_str_equal (key, OPT_CTW) && value->type == GCONF_VALUE_BOOL) {
	mt->dwell_show_ctw = gconf_value_get_bool (value);
	mt_ctw_update_visibility (mt);
    }
    else if (g_str_equal (key, OPT_MODE) && value->type == GCONF_VALUE_INT) {
	mt->dwell_mode = gconf_value_get_int (value);
	mt_ctw_update_sensitivity (mt);
    }
    else if (g_str_equal (key, OPT_STYLE) && value->type == GCONF_VALUE_INT) {
	mt->style = gconf_value_get_int (value);
	mt_ctw_update_style (mt->style);
    }
    else if (g_str_equal (key, OPT_G_SINGLE) && value->type == GCONF_VALUE_INT)
	mt->dwell_dirs[DWELL_CLICK_TYPE_SINGLE] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_G_DOUBLE) && value->type == GCONF_VALUE_INT)
	mt->dwell_dirs[DWELL_CLICK_TYPE_DOUBLE] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_G_DRAG) && value->type == GCONF_VALUE_INT)
	mt->dwell_dirs[DWELL_CLICK_TYPE_DRAG] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_G_RIGHT) && value->type == GCONF_VALUE_INT)
	mt->dwell_dirs[DWELL_CLICK_TYPE_RIGHT] = gconf_value_get_int (value);
    else if (g_str_equal (key, OPT_ANIMATE) && value->type == GCONF_VALUE_BOOL) {
	MtCursorManager *manager;

	manager = mt_cursor_manager_get_default ();
	mt->animate_cursor = gconf_value_get_bool (value);

	if (mt->animate_cursor)
	    mt->cursor = mt_cursor_manager_current_cursor (manager);
	else
	    mt_cursor_manager_restore_all (manager);
    }
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
    mt->animate_cursor = gconf_client_get_bool (mt->client, OPT_ANIMATE, NULL);

    val = gconf_client_get_float (mt->client, OPT_DELAY_T, NULL);
    mt_timer_set_target (mt->delay_timer, val);
    val = gconf_client_get_float (mt->client, OPT_DWELL_T, NULL);
    mt_timer_set_target (mt->dwell_timer, val);

    mt->dwell_dirs[DWELL_CLICK_TYPE_SINGLE] =
	gconf_client_get_int (mt->client, OPT_G_SINGLE, NULL);
    mt->dwell_dirs[DWELL_CLICK_TYPE_DOUBLE] =
	gconf_client_get_int (mt->client, OPT_G_DOUBLE, NULL);
    mt->dwell_dirs[DWELL_CLICK_TYPE_DRAG] =
	gconf_client_get_int (mt->client, OPT_G_DRAG, NULL);
    mt->dwell_dirs[DWELL_CLICK_TYPE_RIGHT] =
	gconf_client_get_int (mt->client, OPT_G_RIGHT, NULL);
}

static gboolean
accessibility_enabled (MTClosure *mt,
		       gint       spi_status)
{
    gboolean a11y;

    a11y = gconf_client_get_bool (mt->client, GNOME_A11Y_KEY, NULL);
    if (!a11y || spi_status != 0) {
	gint ret;

	ret = mt_common_show_dialog
	    (_("Assistive Technology Support Is Not Enabled"),
	     _("Mousetweaks requires assistive technologies to be enabled "
	       "in your session."
	       "\n\n"
	       "To enable support for assistive technologies and restart "
	       "your session, press \"Enable and Log Out\"."),
	     MT_MESSAGE_LOGOUT);
	if (ret == GTK_RESPONSE_ACCEPT) {
	    GnomeClient *session;

	    gconf_client_set_bool (mt->client, GNOME_A11Y_KEY, TRUE, NULL);
	    if ((session = gnome_master_client ()))
		gnome_client_request_save (session, GNOME_SAVE_GLOBAL, TRUE,
					   GNOME_INTERACT_ANY, FALSE, TRUE);
	}
	else {
	    /* reset the selected option again */
	    gconf_client_set_bool (mt->client, OPT_DELAY, FALSE, NULL);
	    gconf_client_set_bool (mt->client, OPT_DWELL, FALSE, NULL);
	}
	return FALSE;
    }
    return TRUE;
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
		      G_CALLBACK (delay_timer_finished), mt);
    g_signal_connect (mt->delay_timer, "tick",
		      G_CALLBACK (delay_timer_tick), mt);

    mt->dwell_timer = mt_timer_new ();
    g_signal_connect (mt->dwell_timer, "finished",
		      G_CALLBACK (dwell_timer_finished), mt);
    g_signal_connect (mt->dwell_timer, "tick",
		      G_CALLBACK (dwell_timer_tick), mt);

    mt->service = mt_service_get_default ();
    mt_service_set_clicktype (mt->service, DWELL_CLICK_TYPE_SINGLE, NULL);

    mt->x_old = -1;
    mt->y_old = -1;

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
    pid_t    pid;
    gboolean delay_click = FALSE;
    gboolean dwell_click = FALSE;
    gdouble  delay_time  = -1.;
    gdouble  dwell_time  = -1.;
    gboolean shutdown    = FALSE;
    gboolean ctw         = FALSE;
    gboolean animate     = FALSE;
    gchar   *mode        = NULL;
    gint     pos_x       = -1;
    gint     pos_y       = -1;
    gint     threshold   = -1;
    GOptionContext *context;
    GOptionEntry entries[] = {
	{"enable-dwell", 0, 0, G_OPTION_ARG_NONE, &dwell_click,
	    _("Enable dwell click"), 0},
	{"enable-secondary", 0, 0, G_OPTION_ARG_NONE, &delay_click,
	    _("Enable simulated secondary click"), 0},
	{"dwell-time", 0, 0, G_OPTION_ARG_DOUBLE, &dwell_time,
	    _("Time to wait before a dwell click"), "[0.5-3.0]"},
	{"secondary-time", 0, 0, G_OPTION_ARG_DOUBLE, &delay_time,
	    _("Time to wait before a simulated secondary click"), "[0.5-3.0]"},
	{"dwell-mode", 'm', 0, G_OPTION_ARG_STRING, &mode,
	    _("Dwell mode to use"), "[window|gesture]"},
	{"show-ctw", 'c', 0, G_OPTION_ARG_NONE, &ctw,
	    _("Show click type window"), 0},
	{"ctw-x", 'x', 0, G_OPTION_ARG_INT, &pos_x,
	    _("Window x position"), 0},
	{"ctw-y", 'y', 0, G_OPTION_ARG_INT, &pos_y,
	    _("Window y position"), 0},
	{"threshold", 't', 0, G_OPTION_ARG_INT, &threshold,
	    _("Ignore small pointer movements"), "[0-30]"},
	{"animate-cursor", 'a', 0, G_OPTION_ARG_NONE, &animate,
	    _("Show elapsed time as cursor overlay"), 0},
	{"shutdown", 's', 0, G_OPTION_ARG_NONE, &shutdown,
	    _("Shut down mousetweaks"), 0},
	{ NULL }
    };

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    context = g_option_context_new (_("- GNOME mousetweaks daemon"));
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
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
    if ((pid = mt_pidfile_is_running ()) >= 0) {
	g_print ("Daemon is already running. (PID %u)\n", pid);
	return 1;
    }
    g_print ("Starting daemon.\n");

    if ((pid = fork ()) < 0) {
	g_print ("Fork failed.\n");
	return 1;
    }
    else if (pid) {
	/* Parent return */
	return 0;
    }
    else {
	/* Child process */
	GnomeProgram *program;
	MTClosure *mt;
	MtCursorManager *manager;
	AccessibleEventListener *bl, *ml;
	gint spi_status;

	if (mt_pidfile_create () < 0)
	    return 1;

	signal (SIGINT, signal_handler);
	signal (SIGTERM, signal_handler);
	signal (SIGQUIT, signal_handler);
	signal (SIGHUP, signal_handler);

	g_set_application_name ("Mousetweaks");
	program = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
				      argc, argv, GNOME_PARAM_NONE);

	mt = mt_closure_init ();
	if (!mt)
	    goto FINISH;

	spi_status = SPI_init ();
	if (!accessibility_enabled (mt, spi_status)) {
	    mt_closure_free (mt);
	    goto FINISH;
	}

	/* add at-spi listeners */
	ml = SPI_createAccessibleEventListener (spi_motion_event, (void *) mt);
	SPI_registerGlobalEventListener (ml, "mouse:abs");
	bl = SPI_createAccessibleEventListener (spi_button_event, (void *) mt);
	SPI_registerGlobalEventListener (bl, "mouse:button:1p");
	SPI_registerGlobalEventListener (bl, "mouse:button:1r");

	/* command-line options */
	if (dwell_click)
	    gconf_client_set_bool (mt->client, OPT_DWELL, TRUE, NULL);
	if (delay_click)
	    gconf_client_set_bool (mt->client, OPT_DELAY, TRUE, NULL);
	if (delay_time >= .5 && delay_time <= 3.)
	    gconf_client_set_float (mt->client, OPT_DELAY_T, delay_time, NULL);
	if (dwell_time >= .5 && dwell_time <= 3.)
	    gconf_client_set_float (mt->client, OPT_DWELL_T, dwell_time, NULL);
	if (threshold >= 0 && threshold <= 30)
	    gconf_client_set_int (mt->client, OPT_THRESHOLD, threshold, NULL);
	if (ctw)
	    gconf_client_set_bool (mt->client, OPT_CTW, TRUE, NULL);
	if (animate)
	    gconf_client_set_bool (mt->client, OPT_ANIMATE, TRUE, NULL);
	if (mode) {
	    if (g_str_equal (mode, "gesture"))
		gconf_client_set_int (mt->client, OPT_MODE,
				      DWELL_MODE_GESTURE, NULL);
	    else if (g_str_equal (mode, "window"))
		gconf_client_set_int (mt->client, OPT_MODE,
				      DWELL_MODE_CTW, NULL);
	    g_free (mode);
	}

	get_gconf_options (mt);

	if (!mt_ctw_init (mt, pos_x, pos_y))
	    goto CLEANUP;

	manager = mt_cursor_manager_get_default ();
	g_signal_connect (manager, "cursor_changed",
			  G_CALLBACK (cursor_changed), mt);
	g_signal_connect (manager, "cache_cleared",
			  G_CALLBACK (cursor_cache_cleared), mt);

	mt->cursor = mt_cursor_manager_current_cursor (manager);

	SPI_event_main ();

	mt_cursor_manager_restore_all (manager);
	g_object_unref (manager);

	CLEANUP:
	    SPI_deregisterGlobalEventListenerAll (bl);
	    AccessibleEventListener_unref (bl);
	    SPI_deregisterGlobalEventListenerAll (ml);
	    AccessibleEventListener_unref (ml);
	    SPI_exit ();
	    mt_closure_free (mt);
	FINISH:
	    mt_pidfile_remove ();
	    g_object_unref (program);
    }
    return 0;
}
