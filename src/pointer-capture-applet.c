/*
 * Copyright © 2007-2009 Gerd Kohlberger <lowfi@chello.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <panel-applet.h>
#include <panel-applet-gconf.h>

#include "mt-common.h"

#define WID(n) (GTK_WIDGET (gtk_builder_get_object (cd->ui, n)))

#define TANGO_CHAMELEON_DARK  0.305f, 0.603f, 0.023f
#define TANGO_SCARLETRED_DARK 0.643f, 0.000f, 0.000f
#define TANGO_ALUMINIUM2_DARK 0.180f, 0.203f, 0.211f

typedef struct _CaptureData CaptureData;
struct _CaptureData {
    PanelApplet *applet;
    GtkBuilder  *ui;
    GtkWidget   *area;

    GdkCursor   *blank;
    GdkCursor   *root;
    gboolean     pointer_locked;
    gint         pointer_x;
    gint         pointer_y;
    gint         center_x;
    gint         center_y;
    gboolean     vertical;

    /* options */
    gint         size;
    gint         cap_button;
    guint        cap_mask;
    gint         rel_button;
    guint        rel_mask;
};

static void capture_data_free (CaptureData *cd);

static void
capture_preferences (BonoboUIComponent *component,
		     gpointer           data,
		     const char        *cname)
{
    CaptureData *cd = data;

    gtk_window_present (GTK_WINDOW (WID ("capture_preferences")));
}

static void
capture_help (BonoboUIComponent *component, gpointer data, const char *cname)
{
    CaptureData *cd = data;

    mt_common_show_help (gtk_widget_get_screen (cd->area),
			 gtk_get_current_event_time ());
}

static void
capture_about (BonoboUIComponent *component, gpointer data, const char *cname)
{
    CaptureData *cd = data;

    gtk_window_present (GTK_WINDOW (WID ("about")));
}

static const BonoboUIVerb menu_verb[] = {
    BONOBO_UI_UNSAFE_VERB ("PropertiesVerb", capture_preferences),
    BONOBO_UI_UNSAFE_VERB ("HelpVerb", capture_help),
    BONOBO_UI_UNSAFE_VERB ("AboutVerb", capture_about),
    BONOBO_UI_VERB_END
};

static void
lock_pointer (CaptureData *cd)
{
    GdkWindow *win;
    gint x, y, w, h;

    /* set invisible cursor */
    win = gdk_screen_get_root_window (gtk_widget_get_screen (cd->area));
    cd->root = gdk_window_get_cursor (win);
    gdk_window_set_cursor (win, cd->blank);

    /* calculate center position */
    win = gtk_widget_get_window (cd->area);
    gdk_drawable_get_size (win, &w, &h);
    gdk_window_get_origin (win, &x, &y);
    cd->center_x = x + (w >> 1);
    cd->center_y = y + (h >> 1);

    /* update state */
    cd->pointer_locked = TRUE;
    gtk_widget_queue_draw (cd->area);
}

static void
unlock_pointer (CaptureData *cd)
{
    GdkWindow *root;
    GdkCursor *cursor;

    /* move pointer to the position where it was locked */
    gdk_display_warp_pointer (gdk_display_get_default (),
			      gtk_widget_get_screen (cd->area),
			      cd->pointer_x,
			      cd->pointer_y);

    /* restore cursor */
    root = gdk_screen_get_root_window (gtk_widget_get_screen (cd->area));
    if (cd->root) {
	gdk_window_set_cursor (root, cd->root);
    }
    else {
	cursor = gdk_cursor_new (GDK_LEFT_PTR);
	gdk_window_set_cursor (root, cursor);
	gdk_cursor_unref (cursor);
    }

    /* update state */
    cd->pointer_locked = FALSE;
    gtk_widget_queue_draw (cd->area);
}

/* lock pointer immediately if we have no button*/
static gboolean
area_entered (GtkWidget *widget, GdkEventCrossing *cev, gpointer data)
{
    CaptureData *cd = data;

    if (cev->mode != GDK_CROSSING_NORMAL)
	return FALSE;

    if (cd->cap_button)
	return FALSE;

    cd->pointer_x = cev->x_root;
    cd->pointer_y = cev->y_root;
    lock_pointer (cd);

    return FALSE;
}

static gboolean
area_left (GtkWidget *widget, GdkEventCrossing *cev, gpointer data)
{
    CaptureData *cd = data;

    if (cd->pointer_locked) {
	/* move pointer back to center */
	gdk_display_warp_pointer (gdk_display_get_default (),
				  gtk_widget_get_screen (widget),
				  cd->center_x,
				  cd->center_y);
    }
    return FALSE;
}

/* change lock states */
static gboolean
area_button_press (GtkWidget *widget, GdkEventButton *bev, gpointer data)
{
    CaptureData *cd = data;

    if (bev->button == cd->cap_button &&
	(bev->state & cd->cap_mask) == cd->cap_mask &&
	!cd->pointer_locked) {

	cd->pointer_x = bev->x_root;
	cd->pointer_y = bev->y_root;
	lock_pointer (cd);
    }
    else if (bev->button == cd->rel_button &&
	     (bev->state & cd->rel_mask) == cd->rel_mask &&
	     cd->pointer_locked) {

	unlock_pointer (cd);
    }
    else if (!cd->pointer_locked && bev->button != 1) {
	g_signal_stop_emission_by_name (widget, "button_press_event");
	return FALSE;
    }

    return TRUE;
}

static void
render_text (cairo_t *cr, CaptureData *cd)
{
    PangoLayout *layout;
    PangoFontDescription *desc;
    PangoRectangle rect;
    guint size;

    layout = pango_cairo_create_layout (cr);

    if (cd->size >= 60)
	pango_layout_set_text (layout, _("Locked"), -1);
    else
	/* l10n: the first letter of 'Locked' */
	pango_layout_set_text (layout, _("L"), -1);

    desc = pango_font_description_new ();
    pango_font_description_set_family (desc, "Bitstream Vera Sans");

    if (cd->vertical)
	pango_font_description_set_gravity (desc, PANGO_GRAVITY_EAST);

    size = panel_applet_get_size (cd->applet) / 3;
    pango_font_description_set_size (desc, size * PANGO_SCALE);

    pango_layout_set_font_description (layout, desc);
    pango_layout_get_pixel_extents (layout, &rect, NULL);

    /* check font size */
    while ((rect.width - 8) > cd->size) {
	pango_font_description_set_size (desc, (--size) * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_layout_get_pixel_extents (layout, &rect, NULL);
    }

    if (cd->vertical)
	cairo_rotate (cr, -pango_gravity_to_rotation (PANGO_GRAVITY_EAST));

    cairo_rel_move_to (cr,
		       -rect.x - rect.width / 2,
		       -rect.y - rect.height / 2);
    pango_cairo_layout_path (cr, layout);

    pango_font_description_free (desc);
    g_object_unref (layout);
}

/* draw background */
static gboolean
area_exposed (GtkWidget *widget, GdkEventExpose *eev, gpointer data)
{
    CaptureData *cd = data;
    cairo_t *cr;
    gdouble w, h;

    w = widget->allocation.width;
    h = widget->allocation.height;

    cr = gdk_cairo_create (widget->window);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle (cr, 0.0, 0.0, w, h);

    if (cd->pointer_locked)
	cairo_set_source_rgb (cr, TANGO_SCARLETRED_DARK);
    else
	cairo_set_source_rgb (cr, TANGO_CHAMELEON_DARK);

    cairo_paint (cr);
    cairo_set_source_rgb (cr, TANGO_ALUMINIUM2_DARK);
    cairo_stroke (cr);

    if (cd->pointer_locked) {
	cairo_move_to (cr, w / 2, h / 2);
	render_text (cr, cd);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_fill (cr);
    }

    cairo_destroy (cr);

    return TRUE;
}

static void
update_orientation (CaptureData *cd, PanelAppletOrient orient)
{
    switch (orient) {
    case PANEL_APPLET_ORIENT_UP:
    case PANEL_APPLET_ORIENT_DOWN:
	gtk_widget_set_size_request (cd->area, cd->size, 0);
	cd->vertical = FALSE;
	break;
    case PANEL_APPLET_ORIENT_LEFT:
    case PANEL_APPLET_ORIENT_RIGHT:
	gtk_widget_set_size_request (cd->area, 0, cd->size);
	cd->vertical = TRUE;
    default:
	break;
    }
}

/* applet callback */
static void
applet_unrealize (GtkWidget *widget, gpointer data)
{
    capture_data_free (data);
}

static void
applet_orient_changed (PanelApplet *applet, guint orient, gpointer data)
{
    update_orientation (data, orient);
}

static void
about_response (GtkButton *button, gint response, gpointer data)
{
    CaptureData *cd = data;

    gtk_widget_hide (WID ("about"));
}

/* preferences dialog callbacks */
static void
prefs_size_changed (GtkSpinButton *spin, gpointer data)
{
    CaptureData *cd = data;

    cd->size = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (cd->applet, "size", cd->size, NULL);

    update_orientation (cd, panel_applet_get_orient (cd->applet));
}

static void
prefs_closed (GtkButton *button, gpointer data)
{
    CaptureData *cd = data;

    gtk_widget_hide (WID ("capture_preferences"));
}

static void
prefs_help (GtkButton *button, gpointer data)
{
    mt_common_show_help (gtk_widget_get_screen (GTK_WIDGET (button)),
			 gtk_get_current_event_time ());
}

static void
prefs_cap_button (GtkSpinButton *spin, gpointer data)
{
    CaptureData *cd = data;

    cd->cap_button = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (cd->applet,
				"capture_button",
				cd->cap_button,
				NULL);
}

static void
prefs_cap_alt (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = data;

    cd->cap_mask ^= GDK_MOD1_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "capture_mod_alt",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_cap_shift (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = data;

    cd->cap_mask ^= GDK_SHIFT_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "capture_mod_shift",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_cap_ctrl (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = data;

    cd->cap_mask ^= GDK_CONTROL_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "capture_mod_ctrl",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_rel_button (GtkSpinButton *spin, gpointer data)
{
    CaptureData *cd = data;

    cd->rel_button = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (cd->applet,
				"release_button",
				cd->rel_button,
				NULL);
}

static void
prefs_rel_alt (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = data;

    cd->rel_mask ^= GDK_MOD1_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "release_mod_alt",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_rel_shift (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = data;

    cd->rel_mask ^= GDK_SHIFT_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "release_mod_shift",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_rel_ctrl (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = data;

    cd->rel_mask ^= GDK_CONTROL_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "release_mod_ctrl",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static gboolean
init_preferences (CaptureData *cd)
{
    GtkWidget *w;
    GError *error = NULL;

    cd->ui = gtk_builder_new ();
    gtk_builder_add_from_file (cd->ui,
			       DATADIR "/pointer-capture-applet.ui",
			       &error);
    if (error) {
	g_print ("%s\n", error->message);
	g_error_free (error);
	return FALSE;
    }

    g_signal_connect (WID ("capture_preferences"), "delete-event",
		      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (WID ("close"), "clicked",
		      G_CALLBACK (prefs_closed), cd);
    g_signal_connect (WID ("help"), "clicked",
		      G_CALLBACK (prefs_help), NULL);

    w = WID ("size");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cd->size);
    g_signal_connect (w, "value_changed",
		      G_CALLBACK (prefs_size_changed), cd);

    /* capture modifier signals */
    w = WID ("cap_button");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cd->cap_button);
    g_signal_connect (w, "value_changed",
		      G_CALLBACK (prefs_cap_button), cd);

    w = WID ("cap_alt");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->cap_mask & GDK_MOD1_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_cap_alt), cd);

    w = WID ("cap_shift");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->cap_mask & GDK_SHIFT_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_cap_shift), cd);

    w = WID ("cap_ctrl");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->cap_mask & GDK_CONTROL_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_cap_ctrl), cd);

    /* release modifier signals */
    w = WID ("rel_button");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cd->rel_button);
    g_signal_connect (w, "value_changed",
		      G_CALLBACK (prefs_rel_button), cd);

    w = WID ("rel_alt");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->rel_mask & GDK_MOD1_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_rel_alt), cd);

    w = WID ("rel_shift");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->rel_mask & GDK_SHIFT_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_rel_shift), cd);

    w = WID ("rel_ctrl");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->rel_mask & GDK_CONTROL_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_rel_ctrl), cd);

    return TRUE;
}

static CaptureData *
capture_data_init (PanelApplet *applet)
{
    CaptureData *cd;

    cd = g_slice_new0 (CaptureData);
    cd->applet = applet;
    cd->size = 100;
    cd->rel_button = 1;
    cd->blank = gdk_cursor_new (GDK_BLANK_CURSOR);

    return cd;
}

static void
capture_data_free (CaptureData *cd)
{
    if (cd->blank)
	gdk_cursor_unref (cd->blank);

    if (cd->ui)
	g_object_unref (cd->ui);

    g_slice_free (CaptureData, cd);
}

static gboolean
fill_applet (PanelApplet *applet)
{
    CaptureData *cd;
    GtkWidget *about;
    AtkObject *obj;

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_application_name (_("Pointer Capture"));
    gtk_window_set_default_icon_name (MT_ICON_NAME);

    cd = capture_data_init (applet);

    /* gconf settings */
    panel_applet_add_preferences (applet,
				  "/schemas/apps/pointer-capture",
				  NULL);

    cd->size = panel_applet_gconf_get_int (applet, "size", NULL);
    cd->cap_button = panel_applet_gconf_get_int (applet,
						 "capture_button", NULL);
    cd->rel_button = panel_applet_gconf_get_int (applet,
						 "release_button", NULL);
    if (panel_applet_gconf_get_bool (applet, "capture_mod_shift", NULL))
	cd->cap_mask |= GDK_SHIFT_MASK;
    if (panel_applet_gconf_get_bool (applet, "capture_mod_ctrl", NULL))
	cd->cap_mask |= GDK_CONTROL_MASK;
    if (panel_applet_gconf_get_bool (applet, "capture_mod_alt", NULL))
	cd->cap_mask |= GDK_MOD1_MASK;
    if (panel_applet_gconf_get_bool (applet, "release_mod_shift", NULL))
	cd->rel_mask |= GDK_SHIFT_MASK;
    if (panel_applet_gconf_get_bool (applet, "release_mod_ctrl", NULL))
	cd->rel_mask |= GDK_CONTROL_MASK;
    if (panel_applet_gconf_get_bool (applet, "release_mod_alt", NULL))
	cd->rel_mask |= GDK_MOD1_MASK;

    /* preferences dialog */
    if (!init_preferences (cd)) {
	capture_data_free (cd);
	return FALSE;
    }

    /* capture area */
    cd->area = gtk_drawing_area_new ();
    gtk_widget_add_events (cd->area,
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK |
			   GDK_BUTTON_PRESS_MASK);

    g_signal_connect (cd->area, "enter-notify-event",
		      G_CALLBACK (area_entered), cd);
    g_signal_connect (cd->area, "leave-notify-event",
		      G_CALLBACK (area_left), cd);
    g_signal_connect (cd->area, "expose-event",
		      G_CALLBACK (area_exposed), cd);
    g_signal_connect (cd->area, "button-press-event",
		      G_CALLBACK (area_button_press), cd);

    update_orientation (cd, panel_applet_get_orient (applet));
    gtk_widget_show (cd->area);

    /* about dialog */
    about = WID ("about");
    g_object_set (about, "version", VERSION, NULL);

    g_signal_connect (about, "delete-event",
		      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (about, "response",
		      G_CALLBACK (about_response), cd);

    /* applet initialisation */
    panel_applet_set_flags (applet,
			    PANEL_APPLET_EXPAND_MINOR |
			    PANEL_APPLET_HAS_HANDLE);
    panel_applet_set_background_widget (applet, GTK_WIDGET (applet));
    panel_applet_setup_menu_from_file (applet,
				       DATADIR, "PointerCapture.xml",
				       NULL, menu_verb, cd);

    g_signal_connect (applet, "change-orient",
		      G_CALLBACK (applet_orient_changed), cd);
    g_signal_connect (applet, "unrealize",
		      G_CALLBACK (applet_unrealize), cd);

    obj = gtk_widget_get_accessible (GTK_WIDGET (applet));
    atk_object_set_name (obj, _("Capture area"));
    atk_object_set_description (obj, _("Temporarily lock the mouse pointer"));

    gtk_container_add (GTK_CONTAINER (applet), cd->area);
    gtk_widget_show (GTK_WIDGET (applet));

    return TRUE;
}

static gboolean
applet_factory (PanelApplet *applet, const gchar *iid, gpointer data)
{
    if (!g_str_equal (iid, "OAFIID:PointerCaptureApplet"))
	return FALSE;

    return fill_applet (applet);
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:PointerCaptureApplet_Factory",
			     PANEL_TYPE_APPLET,
			     "Pointer Capture Factory",
			     VERSION,
			     applet_factory,
			     NULL);
