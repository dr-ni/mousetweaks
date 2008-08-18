/*
 * Copyright © 2007-2008 Gerd Kohlberger <lowfi@chello.at>
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
#include <glade/glade.h>
#include <panel-applet.h>
#include <panel-applet-gconf.h>

#include "mt-common.h"

#define TANGO_CHAMELEON_DARK  0.305f, 0.603f, 0.023f
#define TANGO_SCARLETRED_DARK 0.643f, 0.000f, 0.000f
#define TANGO_ALUMINIUM2_DARK 0.180f, 0.203f, 0.211f

typedef struct _CaptureData CaptureData;
struct _CaptureData {
    PanelApplet *applet;

    GladeXML  *xml;
    GtkWidget *area;
    GtkWidget *prefs;

    GdkCursor *null_cursor;
    gboolean   pointer_locked;
    gint       pointer_x;
    gint       pointer_y;
    gboolean   vertical;

    gint  size;
    gint  cap_button;
    guint cap_mask;
    gint  rel_button;
    guint rel_mask;
};

static void fini_capture_data (CaptureData *cd);

static void
capture_preferences (BonoboUIComponent *component,
		     gpointer           data,
		     const char        *cname)
{
    CaptureData *cd = data;

    gtk_window_present (GTK_WINDOW (cd->prefs));
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

    gtk_window_present (GTK_WINDOW (glade_xml_get_widget (cd->xml, "about")));
}

static const BonoboUIVerb menu_verb[] = {
    BONOBO_UI_UNSAFE_VERB ("PropertiesVerb", capture_preferences),
    BONOBO_UI_UNSAFE_VERB ("HelpVerb", capture_help),
    BONOBO_UI_UNSAFE_VERB ("AboutVerb", capture_about),
    BONOBO_UI_VERB_END
};

/* grab pointer and update lock state */
static gboolean
lock_pointer (CaptureData *cd, guint32 time)
{
    GdkGrabStatus stat;

    stat = gdk_pointer_grab (cd->area->window,
			     FALSE,
			     GDK_BUTTON_PRESS_MASK,
			     cd->area->window,
			     cd->null_cursor,
			     time);

    if (stat != GDK_GRAB_SUCCESS)
	return FALSE;

    cd->pointer_locked = TRUE;
    gtk_widget_queue_draw (cd->area);

    return TRUE;
}

static void
unlock_pointer (CaptureData *cd, guint32 time)
{
    /* move pointer to the position where it was locked */
    gdk_display_warp_pointer (gdk_display_get_default (),
			      gtk_widget_get_screen (cd->area),
			      cd->pointer_x,
			      cd->pointer_y);

    gdk_pointer_ungrab (time);

    cd->pointer_locked = FALSE;
    gtk_widget_queue_draw (cd->area);
}

/* lock pointer immediately if we have no button*/
static gboolean
area_entered (GtkWidget *widget, GdkEventCrossing *cev, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    if (cev->mode != GDK_CROSSING_NORMAL)
	return FALSE;

    if (cd->cap_button)
	return FALSE;

    cd->pointer_x = cev->x_root;
    cd->pointer_y = cev->y_root;

    return lock_pointer (cd, cev->time);
}

/* change lock states */
static gboolean
area_button_press (GtkWidget *widget, GdkEventButton *bev, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    if (bev->button == cd->cap_button &&
	(bev->state & cd->cap_mask) == cd->cap_mask &&
	!cd->pointer_locked) {

	cd->pointer_x = bev->x_root;
	cd->pointer_y = bev->y_root;
	lock_pointer (cd, bev->time);
    }
    else if (bev->button == cd->rel_button &&
	     (bev->state & cd->rel_mask) == cd->rel_mask &&
	     cd->pointer_locked) {

	unlock_pointer (cd, bev->time);
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
    CaptureData *cd = (CaptureData *) data;
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
    fini_capture_data ((CaptureData *) data);
}

static void
applet_orient_changed (PanelApplet *applet, guint orient, gpointer data)
{
    update_orientation ((CaptureData *) data, orient);
}

static void
about_response (GtkButton *button, gint response, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    gtk_widget_hide (glade_xml_get_widget (cd->xml, "about"));
}

/* preferences dialog callbacks */
static void
prefs_size_changed (GtkSpinButton *spin, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->size = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (cd->applet, "size", cd->size, NULL);

    update_orientation (cd, panel_applet_get_orient (cd->applet));
}

static void
prefs_closed (GtkButton *button, gpointer data)
{
    gtk_widget_hide (((CaptureData *) data)->prefs);
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
    CaptureData *cd = (CaptureData *) data;

    cd->cap_button = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (cd->applet,
				"capture_button",
				cd->cap_button,
				NULL);
}

static void
prefs_cap_alt (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->cap_mask ^= GDK_MOD1_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "capture_mod_alt",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_cap_shift (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->cap_mask ^= GDK_SHIFT_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "capture_mod_shift",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_cap_ctrl (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->cap_mask ^= GDK_CONTROL_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "capture_mod_ctrl",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_rel_button (GtkSpinButton *spin, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->rel_button = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (cd->applet,
				"release_button",
				cd->rel_button,
				NULL);
}

static void
prefs_rel_alt (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->rel_mask ^= GDK_MOD1_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "release_mod_alt",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_rel_shift (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

    cd->rel_mask ^= GDK_SHIFT_MASK;
    panel_applet_gconf_set_bool (cd->applet,
				 "release_mod_shift",
				 gtk_toggle_button_get_active (toggle),
				 NULL);
}

static void
prefs_rel_ctrl (GtkToggleButton *toggle, gpointer data)
{
    CaptureData *cd = (CaptureData *) data;

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

    cd->xml = glade_xml_new (DATADIR "/pointer-capture-applet.glade",
			     NULL, NULL);
    if (!cd->xml)
	return FALSE;

    cd->prefs = glade_xml_get_widget (cd->xml, "capture_preferences");
    g_signal_connect (cd->prefs, "delete-event",
		      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    w = glade_xml_get_widget (cd->xml, "close");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (prefs_closed), cd);

    w = glade_xml_get_widget (cd->xml, "help");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (prefs_help), NULL);

    w = glade_xml_get_widget (cd->xml, "size");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cd->size);
    g_signal_connect (w, "value_changed",
		      G_CALLBACK (prefs_size_changed), cd);

    /* capture modifier signals */
    w = glade_xml_get_widget (cd->xml, "cap_button");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cd->cap_button);
    g_signal_connect (w, "value_changed",
		      G_CALLBACK (prefs_cap_button), cd);
    w = glade_xml_get_widget (cd->xml, "cap_alt");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->cap_mask & GDK_MOD1_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_cap_alt), cd);
    w = glade_xml_get_widget (cd->xml, "cap_shift");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->cap_mask & GDK_SHIFT_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_cap_shift), cd);
    w = glade_xml_get_widget (cd->xml, "cap_ctrl");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->cap_mask & GDK_CONTROL_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_cap_ctrl), cd);

    /* release modifier signals */
    w = glade_xml_get_widget (cd->xml, "rel_button");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), cd->rel_button);
    g_signal_connect (w, "value_changed",
		      G_CALLBACK (prefs_rel_button), cd);
    w = glade_xml_get_widget (cd->xml, "rel_alt");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->rel_mask & GDK_MOD1_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_rel_alt), cd);
    w = glade_xml_get_widget (cd->xml, "rel_shift");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->rel_mask & GDK_SHIFT_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_rel_shift), cd);
    w = glade_xml_get_widget (cd->xml, "rel_ctrl");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				  (cd->rel_mask & GDK_CONTROL_MASK));
    g_signal_connect (w, "toggled",
		      G_CALLBACK (prefs_rel_ctrl), cd);

    return TRUE;
}

static CaptureData *
init_capture_data (PanelApplet *applet)
{
    CaptureData *cd;

    cd = g_slice_new0 (CaptureData);
    if (!cd)
	return NULL;

    cd->applet = applet;
    cd->size = 100;
    cd->rel_button = 1;

    return cd;
}

static void
fini_capture_data (CaptureData *cd)
{
    if (cd->null_cursor)
	gdk_cursor_unref (cd->null_cursor);

    g_slice_free (CaptureData, cd);
}

static gboolean
fill_applet (PanelApplet *applet)
{
    CaptureData *cd;
    GtkWidget *about;
    GdkPixmap *bmp0 = NULL;
    gchar char0[] = { 0x0 };
    GdkColor c0 = { 0, 0, 0, 0 };
    AtkObject *obj;

    cd = init_capture_data (applet);
    if (!cd)
	return FALSE;

    /* invisible cursor */
    bmp0 = gdk_bitmap_create_from_data (NULL, char0, 1, 1);
    if (!bmp0) {
	g_free (cd);
	return FALSE;
    }

    cd->null_cursor = gdk_cursor_new_from_pixmap (bmp0, bmp0, &c0, &c0, 0, 0);
    g_object_unref (bmp0);

    if (!cd->null_cursor) {
	g_free (cd);
	return FALSE;
    }

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_application_name (_("Pointer Capture Applet"));
    gtk_window_set_default_icon_name (MT_ICON_NAME);

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
	fini_capture_data (cd);
	return FALSE;
    }

    /* capture area */
    cd->area = gtk_drawing_area_new ();
    gtk_widget_add_events (cd->area,
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_BUTTON_PRESS_MASK);

    g_signal_connect (cd->area, "enter-notify-event",
		      G_CALLBACK (area_entered), cd);
    g_signal_connect (cd->area, "expose-event",
		      G_CALLBACK (area_exposed), cd);
    g_signal_connect (cd->area, "button-press-event",
		      G_CALLBACK (area_button_press), cd);

    update_orientation (cd, panel_applet_get_orient (applet));
    gtk_widget_show (cd->area);

    /* about dialog */
    about = glade_xml_get_widget (cd->xml, "about");
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
    gtk_container_set_border_width (GTK_CONTAINER (applet), 1);
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
