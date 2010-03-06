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

#include <signal.h>
#include <unistd.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cspi/spi.h>
#include <dbus/dbus-glib.h>
#include <X11/extensions/XTest.h>

#include "mt-common.h"
#include "mt-service.h"
#include "mt-pidfile.h"
#include "mt-ctw.h"
#include "mt-timer.h"
#include "mt-cursor-manager.h"
#include "mt-cursor.h"
#include "mt-main.h"
#include "mt-listener.h"
#include "mt-accessible.h"

#define GSM_DBUS_NAME      "org.gnome.SessionManager"
#define GSM_DBUS_PATH      "/org/gnome/SessionManager"
#define GSM_DBUS_INTERFACE "org.gnome.SessionManager"

enum {
    PRESS = 0,
    RELEASE,
    CLICK,
    DOUBLE_CLICK
};

typedef struct _MtCliArgs {
    gdouble  delay_time;
    gdouble  dwell_time;
    gchar   *mode;
    gint     pos_x;
    gint     pos_y;
    gint     threshold;
    gboolean delay_click;
    gboolean dwell_click;
    gboolean shutdown;
    gboolean daemonize;
    gboolean ctw;
    gboolean no_animation;
    gboolean login;
} MtCliArgs;

static GdkScreen *
mt_main_current_screen (MtData *mt)
{
    GdkScreen *screen;

    if (mt->n_screens > 1)
	gdk_display_get_pointer (gdk_display_get_default (),
				 &screen, NULL, NULL, NULL);
    else
	screen = gdk_screen_get_default ();

    return screen;
}

static void
mt_main_generate_motion_event (GdkScreen *screen, gint x, gint y)
{
    gdk_error_trap_push ();
    gdk_display_warp_pointer (gdk_display_get_default (), screen, x, y);
    gdk_flush ();
    gdk_error_trap_pop ();
}

static guint
mt_main_check_mouse_orientation (MtData *mt, guint button)
{
    if (mt->left_handed) {
	if (button == 1)
	    return 3;
	else if (button == 3)
	    return 1;
    }
    return button;
}

static void
mt_main_generate_button_event (MtData *mt,
			       guint   button,
			       gint    type,
			       gulong  delay)
{
    button = mt_main_check_mouse_orientation (mt, button);

    gdk_error_trap_push ();
    switch (type) {
	case PRESS:
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, True, delay);
	    break;
	case RELEASE:
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, False, delay);
	    break;
	case CLICK:
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, True, CurrentTime);
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, False, delay);
	    break;
	case DOUBLE_CLICK:
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, True, CurrentTime);
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, False, delay);
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, True, delay);
	    XTestFakeButtonEvent (mt->xtst_display,
				  button, False, delay);
	    break;
	default:
	    g_warning ("Unknown sequence.");
	    break;
    }
    XFlush (mt->xtst_display);
    gdk_error_trap_pop ();
}

static void
mt_main_set_cursor (MtData *mt, GdkCursorType type)
{
    GdkScreen *screen;
    GdkCursor *cursor;
    gint i;

    cursor = gdk_cursor_new (type);
    for (i = 0; i < mt->n_screens; ++i) {
	screen = gdk_display_get_screen (gdk_display_get_default (), i);
	gdk_window_set_cursor (gdk_screen_get_root_window (screen), cursor);
    }
    gdk_cursor_unref (cursor);
}

static void
dwell_restore_single_click (MtData *mt)
{
    if (mt->dwell_mode == DWELL_MODE_CTW)
	mt_ctw_set_clicktype (mt, DWELL_CLICK_TYPE_SINGLE);

    mt_service_set_clicktype (mt->service, DWELL_CLICK_TYPE_SINGLE, NULL);
}

static void
mt_main_do_dwell_click (MtData *mt)
{
    guint clicktype;

    clicktype = mt_service_get_clicktype (mt->service);

    if (mt->dwell_mode == DWELL_MODE_GESTURE && !mt->dwell_drag_started)
	mt_main_generate_motion_event (mt_main_current_screen (mt),
				       mt->pointer_x, mt->pointer_y);

    switch (clicktype) {
	case DWELL_CLICK_TYPE_SINGLE:
	    mt_main_generate_button_event (mt, 1, CLICK, 60);
	    break;
	case DWELL_CLICK_TYPE_DOUBLE:
	    mt_main_generate_button_event (mt, 1, DOUBLE_CLICK, 10);
	    dwell_restore_single_click (mt);
	    break;
	case DWELL_CLICK_TYPE_DRAG:
	    if (!mt->dwell_drag_started) {
		mt_main_generate_button_event (mt, 1, PRESS, CurrentTime);
		mt_main_set_cursor (mt, GDK_FLEUR);
		mt->dwell_drag_started = TRUE;
	    }
	    else {
		mt_main_generate_button_event (mt, 1, RELEASE, CurrentTime);
		mt_main_set_cursor (mt, GDK_LEFT_PTR);
		mt->dwell_drag_started = FALSE;
		dwell_restore_single_click (mt);
	    }
	    break;
	case DWELL_CLICK_TYPE_RIGHT:
	    mt_main_generate_button_event (mt, 3, CLICK, 10);
	    dwell_restore_single_click (mt);
	    break;
	default:
	    g_warning ("Unknown click-type.");
	    break;
    }
}

static inline gboolean
below_threshold (MtData *mt, gint x, gint y)
{
    gint dx, dy;

    dx = x - mt->pointer_x;
    dy = y - mt->pointer_y;

    return (dx * dx + dy * dy) < (mt->threshold * mt->threshold);
}

static gboolean
mt_main_analyze_gesture (MtData *mt)
{
    gint x, y, gd, i, dx, dy;

    if (mt_service_get_clicktype (mt->service) == DWELL_CLICK_TYPE_DRAG)
	return TRUE;

    gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
    if (below_threshold (mt, x, y))
	return FALSE;

    dx = ABS (x - mt->pointer_x);
    dy = ABS (y - mt->pointer_y);

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
    for (i = 0; i < N_CLICK_TYPES; i++) {
	if (mt->dwell_dirs[i] == gd) {
	    mt_service_set_clicktype (mt->service, i, NULL);
	    return TRUE;
	}
    }
    return FALSE;
}

static void
mt_main_draw_line (MtData *mt, gint x1, gint y1, gint x2, gint y2)
{
    GdkWindow *root;
    GdkGC *gc;

    root = gdk_screen_get_root_window (mt_main_current_screen (mt));
    gc = gdk_gc_new (root);
    gdk_gc_set_subwindow (gc, GDK_INCLUDE_INFERIORS);
    gdk_gc_set_function (gc, GDK_INVERT);
    gdk_gc_set_line_attributes (gc, 1,
				GDK_LINE_SOLID,
				GDK_CAP_ROUND,
				GDK_JOIN_ROUND);
    gdk_draw_arc (root, gc, TRUE, x1 - 4, y1 - 4, 8, 8, 0, 23040);
    gdk_draw_line (root, gc, x1, y1, x2, y2);
    g_object_unref (gc);
}

static void
dwell_start_gesture (MtData *mt)
{
    GdkCursor *cursor;
    GdkWindow *root;

    if (mt->override_cursor) {
	cursor = gdk_cursor_new (GDK_CROSS);
	root = gdk_screen_get_root_window (mt_main_current_screen (mt));
	gdk_pointer_grab (root, FALSE,
			  GDK_POINTER_MOTION_MASK, 
			  NULL, cursor,
			  gtk_get_current_event_time ());
	gdk_cursor_unref (cursor);
    }
    else {
	mt_main_set_cursor (mt, GDK_CROSS);
    }

    mt->dwell_gesture_started = TRUE;
    mt_timer_start (mt->dwell_timer);
}

static void
dwell_stop_gesture (MtData *mt)
{
    if (mt->override_cursor)
	gdk_pointer_ungrab (gtk_get_current_event_time ());
    else
	mt_main_set_cursor (mt, GDK_LEFT_PTR);

    if (mt->x_old > -1 && mt->y_old > -1) {
	mt_main_draw_line (mt, 
			   mt->pointer_x, mt->pointer_y,
			   mt->x_old, mt->y_old);
	mt->x_old = -1;
	mt->y_old = -1;
    }

    mt->dwell_gesture_started = FALSE;
    mt_timer_stop (mt->dwell_timer);
}

static void
dwell_timer_finished (MtTimer *timer, gpointer data)
{
    MtData *mt = data;

    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

    if (mt->dwell_mode == DWELL_MODE_CTW) {
	mt_main_do_dwell_click (mt);
    }
    else {
	if (mt->dwell_gesture_started) {
	    dwell_stop_gesture (mt);

	    if (mt_main_analyze_gesture (mt))
		mt_main_do_dwell_click (mt);
	}
	/* if a drag action is in progress stop it */
	else if (mt->dwell_drag_started) {
	    mt_main_do_dwell_click (mt);
	}
	else
	    dwell_start_gesture (mt);
    }
}

static gboolean
eval_func (Accessible *a, gpointer data)
{
    gchar *name;
    gboolean found = FALSE;

    name = Accessible_getName (a);
    if (name) {
	found = g_str_equal (name, "Window List");
	SPI_freeString (name);
    }
    return found;
}

static gboolean
push_func (Accessible *a, gpointer data)
{
    MtData *mt = data;
    AccessibleRole role;

    role = Accessible_getRole (a);
    if (role != SPI_ROLE_PANEL && role != SPI_ROLE_EMBEDDED)
	return FALSE;

    if (!mt_accessible_is_visible (a))
	return FALSE;

    if (Accessible_isComponent (a))
	return mt_accessible_in_extents (a, mt->pointer_x, mt->pointer_y);

    return TRUE;
}

static gboolean
mt_main_use_move_release (MtData *mt)
{
    Accessible *point, *search;

    point = mt_accessible_at_point (mt->pointer_x, mt->pointer_y);
    if (point) {
	search = mt_accessible_search (point,
				       MT_SEARCH_TYPE_BREADTH,
				       eval_func, push_func, mt);
	Accessible_unref (point);
	if (search) {
	    Accessible_unref (search);
	    return TRUE;
	}
    }
    return FALSE;
}

static void
delay_timer_finished (MtTimer *timer, MtData *mt)
{
    /* set secondary-click flag */
    mt->delay_finished = TRUE;

    if (mt->move_release) {
	mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());
	mt_main_generate_button_event (mt, 3, CLICK, CurrentTime);
    }
}

static void
mt_main_do_secondary_click (MtData *mt)
{
    mt->delay_finished = FALSE;
    mt_main_generate_button_event (mt, 3, CLICK, CurrentTime);
}

static void
mt_dwell_click_cancel (MtData *mt)
{
    if (mt->dwell_gesture_started) {
	dwell_stop_gesture (mt);
	return;
    }

    mt_timer_stop (mt->dwell_timer);
    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

    if (mt->dwell_drag_started) {
	mt_main_set_cursor (mt, GDK_LEFT_PTR);
	mt->dwell_drag_started = FALSE;
    }

    dwell_restore_single_click (mt);
}

/* at-spi listener callbacks */
static void
global_motion_event (MtListener *listener,
		     MtEvent    *event,
		     gpointer    data)
{
    MtData *mt = data;

    if (mt->delay_enabled) {
	if (!below_threshold (mt, event->x, event->y)) {
	    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

	    if (mt_timer_is_running (mt->delay_timer))
		mt_timer_stop (mt->delay_timer);

	    if (mt->delay_finished)
		mt->delay_finished = FALSE;
	}
    }

    if (mt->dwell_enabled) {
	if (!below_threshold (mt, event->x, event->y) &&
	    !mt->dwell_gesture_started) {
	    mt->pointer_x = event->x;
	    mt->pointer_y = event->y;
	    mt_timer_start (mt->dwell_timer);
	}

	if (mt->dwell_gesture_started) {
	    if (mt->x_old > -1 && mt->y_old > -1) {
		mt_main_draw_line (mt,
				   mt->pointer_x, mt->pointer_y,
				   mt->x_old, mt->y_old);
	    }
	    mt_main_draw_line (mt, 
			       mt->pointer_x, mt->pointer_y,
			       event->x, event->y);
	    mt->x_old = event->x;
	    mt->y_old = event->y;
	}
    }
}

static void
global_button_event (MtListener *listener,
		     MtEvent    *event,
		     gpointer    data)
{
    MtData *mt = data;

    if (mt->delay_enabled && event->button == 1) {
	if (event->type == EV_BUTTON_PRESS) {
	    mt->pointer_x = event->x;
	    mt->pointer_y = event->y;
	    mt_timer_start (mt->delay_timer);
	}
	else {
	    mt_cursor_manager_restore_all (mt_cursor_manager_get_default ());

	    if (mt->delay_finished)
		mt_main_do_secondary_click (mt);
	    else
		mt_timer_stop (mt->delay_timer);
	}
    }
    /*
     * cancel a dwell-click in progress if a physical button
     * is pressed - useful for mixed use-cases and testing
     */
    if ((event->type == EV_BUTTON_PRESS && mt_timer_is_running (mt->dwell_timer)) ||
        (event->type == EV_BUTTON_RELEASE && mt->dwell_drag_started)) {
	mt_dwell_click_cancel (mt);
    }
}

static void
global_focus_event (MtListener *listener,
		    gpointer    data)
{
    MtData *mt = data;
    Accessible *accessible;

    if (mt->delay_enabled) {
	accessible = mt_listener_current_focus (listener);
	/* TODO: check for more objects and conditions.
	 * Some links don't have jump actions, eg: text-mails in thunderbird.
	 */
	mt->move_release = mt_accessible_supports_action (accessible, "jump");
    }
}

static gboolean
cursor_overlay_time (MtData  *mt,
		     guchar  *image,
		     gint     width,
		     gint     height,
		     MtTimer *timer,
		     gdouble  time)
{
    GtkWidget *ctw;
    GdkColor c;
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

    ctw = mt_ctw_get_window (mt);
    c = ctw->style->bg[GTK_STATE_SELECTED];
    target = mt_timer_get_target (timer);

    cairo_set_operator (cr, CAIRO_OPERATOR_ATOP);
    cairo_rectangle (cr, 0, 0, width, height / (target / time));
    cairo_set_source_rgba (cr,
			   c.red   / 65535.,
			   c.green / 65535.,
			   c.blue  / 65535.,
			   0.60);
    cairo_fill (cr);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return TRUE;
}

static void
mt_main_update_cursor (MtData  *mt,
		       MtTimer *timer,
		       gdouble  time)
{
    guchar *image;
    gushort width, height;

    image = mt_cursor_get_image_copy (mt->cursor);
    if (!image)
	return;

    mt_cursor_get_dimension (mt->cursor, &width, &height);

    if (cursor_overlay_time (mt, image, width, height, timer, time)) {
	MtCursorManager *manager;
	MtCursor *new_cursor;
	const gchar *name;
	gushort xhot, yhot;

	name = mt_cursor_get_name (mt->cursor);
	mt_cursor_get_hotspot (mt->cursor, &xhot, &yhot);
	new_cursor = mt_cursor_new (name, image, width, height, xhot, yhot);
	manager = mt_cursor_manager_get_default ();
	mt_cursor_manager_set_cursor (manager, new_cursor);
	g_object_unref (new_cursor);
    }
    g_free (image);
}

static void
mt_main_timer_tick (MtTimer *timer,
		    gdouble  time,
		    gpointer data)
{
    MtData *mt = data;

    if (mt->animate_cursor && mt->cursor)
	mt_main_update_cursor (mt, timer, time);
}

static void
cursor_cache_cleared (MtCursorManager *manager,
		      gpointer         data)
{
    MtData *mt = data;

    mt->cursor = mt_cursor_manager_current_cursor (manager);
}

static void
cursor_changed (MtCursorManager *manager,
		const gchar     *name,
		gpointer         data)
{
    MtData *mt = data;

    if (!mt->dwell_gesture_started)
	mt->override_cursor = !g_str_equal (name, "left_ptr");

    mt->cursor = mt_cursor_manager_lookup_cursor (manager, name);
}

static void
signal_handler (int sig)
{
    gtk_main_quit ();
}

static void
gconf_value_changed (GConfClient *client,
		     const gchar *key,
		     GConfValue  *value,
		     gpointer     data)
{
    MtData *mt = data;

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
	mt_ctw_update_style (mt, mt->style);
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
    else if (g_str_equal (key, GNOME_MOUSE_ORIENT) &&
	     value->type == GCONF_VALUE_BOOL) {
	mt->left_handed = gconf_value_get_bool (value);
    }
}

static void
get_gconf_options (MtData *mt)
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

    /* mouse orientation */
    mt->left_handed = gconf_client_get_bool (mt->client, GNOME_MOUSE_ORIENT, NULL);
}

static void
mt_main_request_logout (MtData *mt)
{
    DBusGConnection *bus;
    DBusGProxy *proxy;

    bus = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
    if (bus) {
	proxy = dbus_g_proxy_new_for_name (bus,
					   GSM_DBUS_NAME,
					   GSM_DBUS_PATH,
					   GSM_DBUS_INTERFACE);
	/*
	 * Call logout method of session manager:
	 * mode: 0 = normal, 1 = no confirmation, 2 = force
	 */
	dbus_g_proxy_call (proxy, "Logout", NULL,
			   G_TYPE_UINT, 1, G_TYPE_INVALID,
			   G_TYPE_INVALID);
	g_object_unref (proxy);
    }
}

static gboolean
accessibility_enabled (MtData *mt,
		       gint    spi_status)
{
    gint ret;

    if (spi_status != 0) {
	ret = mt_common_show_dialog
	    (_("Assistive Technology Support is not Enabled"),
	     _("Mousetweaks requires assistive technologies to be enabled "
	       "in your session."
	       "\n\n"
	       "To enable support for assistive technologies and restart "
	       "your session, press \"Enable and Log Out\"."),
	     MT_MESSAGE_LOGOUT);

	if (ret == GTK_RESPONSE_ACCEPT) {
	    gconf_client_set_bool (mt->client, GNOME_A11Y_KEY, TRUE, NULL);
	    mt_main_request_logout (mt);
	}
	else {
	    /* reset the selected option again */
	    if (gconf_client_get_bool (mt->client, OPT_DELAY, NULL))
		gconf_client_set_bool (mt->client, OPT_DELAY, FALSE, NULL);
	    if (gconf_client_get_bool (mt->client, OPT_DWELL, NULL))
		gconf_client_set_bool (mt->client, OPT_DWELL, FALSE, NULL);
	}
	return FALSE;
    }
    return TRUE;
}

static MtData *
mt_data_init (void)
{
    MtData *mt;
    gint ev_base, err_base, maj, min;

    mt = g_slice_new0 (MtData);
    mt->xtst_display = XOpenDisplay (NULL);

    if (!XTestQueryExtension (mt->xtst_display,
			      &ev_base, &err_base, &maj, &min)) {
	XCloseDisplay (mt->xtst_display);
	g_slice_free (MtData, mt);
	g_critical ("No XTest extension found. Aborting..");
	return NULL;
    }

    mt->client = gconf_client_get_default ();
    gconf_client_add_dir (mt->client, GNOME_MOUSE_DIR,
			  GCONF_CLIENT_PRELOAD_NONE, NULL);
    gconf_client_add_dir (mt->client, MT_GCONF_HOME,
			  GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
    g_signal_connect (mt->client, "value_changed",
		      G_CALLBACK (gconf_value_changed), mt);

    mt->delay_timer = mt_timer_new ();
    g_signal_connect (mt->delay_timer, "finished",
		      G_CALLBACK (delay_timer_finished), mt);
    g_signal_connect (mt->delay_timer, "tick",
		      G_CALLBACK (mt_main_timer_tick), mt);

    mt->dwell_timer = mt_timer_new ();
    g_signal_connect (mt->dwell_timer, "finished",
		      G_CALLBACK (dwell_timer_finished), mt);
    g_signal_connect (mt->dwell_timer, "tick",
		      G_CALLBACK (mt_main_timer_tick), mt);

    mt->service = mt_service_get_default ();
    mt_service_set_clicktype (mt->service, DWELL_CLICK_TYPE_SINGLE, NULL);

    mt->n_screens = gdk_display_get_n_screens (gdk_display_get_default ());

    mt->x_old = -1;
    mt->y_old = -1;

    return mt;
}

static void
mt_data_free (MtData *mt)
{
    g_object_unref (mt->delay_timer);
    g_object_unref (mt->dwell_timer);
    g_object_unref (mt->service);
    g_object_unref (mt->client);

    if (mt->ui) {
	gtk_widget_destroy (mt_ctw_get_window (mt));
	g_object_unref (mt->ui);
    }

    g_slice_free (MtData, mt);
}

static MtCliArgs
mt_parse_options (int *argc, char ***argv)
{
    MtCliArgs ca;
    GOptionContext *context;
    GOptionEntry entries[] = {
	{"enable-dwell", 0, 0, G_OPTION_ARG_NONE, &ca.dwell_click,
	    N_("Enable dwell click"), 0},
	{"enable-secondary", 0, 0, G_OPTION_ARG_NONE, &ca.delay_click,
	    N_("Enable simulated secondary click"), 0},
	{"dwell-time", 0, 0, G_OPTION_ARG_DOUBLE, &ca.dwell_time,
	    N_("Time to wait before a dwell click"), "[0.2-3.0]"},
	{"secondary-time", 0, 0, G_OPTION_ARG_DOUBLE, &ca.delay_time,
	    N_("Time to wait before a simulated secondary click"), "[0.5-3.0]"},
	{"dwell-mode", 'm', 0, G_OPTION_ARG_STRING, &ca.mode,
	    N_("Set the active dwell mode"), "[window|gesture]"},
	{"show-ctw", 'c', 0, G_OPTION_ARG_NONE, &ca.ctw,
	    N_("Show a click-type window"), 0},
	{"ctw-x", 'x', 0, G_OPTION_ARG_INT, &ca.pos_x,
	    N_("Click-type window X position"), 0},
	{"ctw-y", 'y', 0, G_OPTION_ARG_INT, &ca.pos_y,
	    N_("Click-type window Y position"), 0},
	{"threshold", 't', 0, G_OPTION_ARG_INT, &ca.threshold,
	    N_("Ignore small pointer movements"), "[0-30]"},
	{"shutdown", 's', 0, G_OPTION_ARG_NONE, &ca.shutdown,
	    N_("Shut down mousetweaks"), 0},
	{"disable-animation", 0, 0, G_OPTION_ARG_NONE, &ca.no_animation,
	    N_("Disable cursor animations"), 0},
	{"daemonize", 0, 0, G_OPTION_ARG_NONE, &ca.daemonize,
	    N_("Start mousetweaks as a daemon"), 0},
	{"login", 0, 0, G_OPTION_ARG_NONE, &ca.login,
	    N_("Start mousetweaks in login mode"), 0},
	{ NULL }
    };
    /* init cli arguments */
    ca.delay_time   = -1.;
    ca.dwell_time   = -1.;
    ca.mode         = NULL;
    ca.pos_x        = -1;
    ca.pos_y        = -1;
    ca.threshold    = -1;
    ca.delay_click  = FALSE;
    ca.dwell_click  = FALSE;
    ca.shutdown     = FALSE;
    ca.daemonize    = FALSE;
    ca.ctw          = FALSE;
    ca.no_animation = FALSE;
    ca.login        = FALSE;
    /* parse */
    context = g_option_context_new (_("- GNOME mouse accessibility daemon"));
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_parse (context, argc, argv, NULL);
    g_option_context_free (context);

    return ca;
}

static void
mt_main (int argc, char **argv, MtCliArgs cli_args)
{
    MtData *mt;
    MtCursorManager *manager;
    MtListener *listener;
    gint spi_status;
    gint spi_leaks = 0;

    if (mt_pidfile_create () < 0) {
	g_warning ("Couldn't create PID file.");
	return;
    }

    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGQUIT, signal_handler);
    signal (SIGHUP, signal_handler);

    gtk_init (&argc, &argv);

    mt = mt_data_init ();
    if (!mt)
	goto FINISH;

    spi_status = SPI_init ();
    /* don't check a11y key in login mode */
    if (!cli_args.login) {
	if (!accessibility_enabled (mt, spi_status)) {
	    mt_data_free (mt);
	    goto FINISH;
	}
    }

    /* load gconf settings */
    get_gconf_options (mt);

    /* override with CLI arguments */
    if (cli_args.dwell_click)
	mt->dwell_enabled = cli_args.dwell_click;
    if (cli_args.delay_click)
	mt->delay_enabled = cli_args.delay_click;
    if (cli_args.dwell_time >= .1 && cli_args.dwell_time <= 3.)
	mt_timer_set_target (mt->dwell_timer, cli_args.dwell_time);
    if (cli_args.delay_time >= .1 && cli_args.delay_time <= 3.)
	mt_timer_set_target (mt->delay_timer, cli_args.delay_time);
    if (cli_args.threshold >= 0 && cli_args.threshold <= 30)
	mt->threshold = cli_args.threshold;
    if (cli_args.ctw)
	mt->dwell_show_ctw = cli_args.ctw;
    if (cli_args.no_animation)
	mt->animate_cursor = !cli_args.no_animation;
    if (cli_args.mode) {
	if (g_str_equal (cli_args.mode, "gesture"))
	    mt->dwell_mode = DWELL_MODE_GESTURE;
	else if (g_str_equal (cli_args.mode, "window"))
	    mt->dwell_mode = DWELL_MODE_CTW;

	g_free (cli_args.mode);
    }

    /* init click-type window */
    if (!mt_ctw_init (mt, cli_args.pos_x, cli_args.pos_y))
	goto CLEANUP;

    /* init cursor animation */
    manager = mt_cursor_manager_get_default ();
    mt->cursor = mt_cursor_manager_current_cursor (manager);
    g_signal_connect (manager, "cursor_changed",
                      G_CALLBACK (cursor_changed), mt);
    g_signal_connect (manager, "cache_cleared",
                      G_CALLBACK (cursor_cache_cleared), mt);

    /* init at-spi signals */
    listener = mt_listener_get_default ();
    g_signal_connect (listener, "motion_event",
                      G_CALLBACK (global_motion_event), mt);
    g_signal_connect (listener, "button_event",
                      G_CALLBACK (global_button_event), mt);
    g_signal_connect (listener, "focus_changed",
                      G_CALLBACK (global_focus_event), mt);

    gtk_main ();

    mt_cursor_manager_restore_all (manager);
    g_object_unref (manager);
    g_object_unref (listener);

CLEANUP:
    spi_leaks = SPI_exit ();
    mt_data_free (mt);
FINISH:
    mt_pidfile_remove ();

    if (spi_leaks)
	g_warning ("AT-SPI reported %i leak%s.",
		   spi_leaks, spi_leaks != 1 ? "s" : "");
}

int
main (int argc, char **argv)
{
    MtCliArgs cli_args;
    pid_t pid;

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    setlocale (LC_ALL, "");

    g_set_application_name ("Mousetweaks");
    cli_args = mt_parse_options (&argc, &argv);

    if (cli_args.shutdown) {
	int ret;

	if ((ret = mt_pidfile_kill_wait (SIGINT, 5)) < 0)
	    g_print ("Shutdown failed or nothing to shut down.\n");
	else
	    g_print ("Shutdown successful.\n");

	return ret < 0 ? 1 : 0;
    }
    if ((pid = mt_pidfile_is_running ()) >= 0) {
	g_print ("Mousetweaks is already running. (PID %u)\n", pid);
	return 1;
    }
    if (cli_args.daemonize) {
	g_print ("Starting daemon.\n");
	if ((pid = fork ()) < 0) {
	    g_error ("fork() failed.");
	    return 1;
	}
	else if (pid)
	    return 0;
    }
    mt_main (argc, argv, cli_args);

    return 0;
}
