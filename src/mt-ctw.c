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

#include <gtk/gtk.h>

#include "mt-main.h"
#include "mt-service.h"
#include "mt-common.h"
#include "mt-ctw.h"

#define WID(n) (GTK_WIDGET (gtk_builder_get_object (mt->ui, n)))

enum {
    BUTTON_STYLE_TEXT = 0,
    BUTTON_STYLE_ICON,
    BUTTON_STYLE_BOTH
};

void
mt_ctw_set_clicktype (MtData *mt, guint clicktype)
{
    GSList *group;
    gpointer data;

    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (WID ("single")));
    data = g_slist_nth_data (group, clicktype);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data), TRUE);
    gtk_widget_grab_focus (GTK_WIDGET (data));
}

void
mt_ctw_update_visibility (MtData *mt)
{
    GtkWidget *ctw;
    GdkScreen *screen;

    ctw = mt_ctw_get_window (mt);

    if (mt->dwell_enabled && mt->dwell_show_ctw) {
	if (mt->n_screens > 1) {
	    gdk_display_get_pointer (gdk_display_get_default (),
				     &screen, NULL, NULL, NULL);
	    gtk_window_set_screen (GTK_WINDOW (ctw), screen);
	}
	gtk_widget_show (ctw);
    }
    else {
	gtk_widget_hide (ctw);
    }
}

void
mt_ctw_update_sensitivity (MtData *mt)
{
    gboolean sensitive;

    sensitive = mt->dwell_enabled && mt->dwell_mode == DWELL_MODE_CTW;
    gtk_widget_set_sensitive (WID ("box"), sensitive);
}

void
mt_ctw_update_style (MtData *mt, gint style)
{
    GtkWidget *icon, *label;
    const gchar *l[] = { "single_l", "double_l", "drag_l", "right_l" };
    const gchar *img[] = { "single_i", "double_i", "drag_i", "right_i" };
    gint i;

    for (i = 0; i < N_CLICK_TYPES; i++) {
	label = WID (l[i]);
	icon = WID (img[i]);

	switch (style) {
	case BUTTON_STYLE_BOTH:
	    g_object_set (icon, "yalign", 1.0, NULL);
	    gtk_widget_show (icon);
	    g_object_set (label, "yalign", 0.0, NULL);
	    gtk_widget_show (label);
	    break;
	case BUTTON_STYLE_TEXT:
	    gtk_widget_hide (icon);
	    g_object_set (icon, "yalign", 0.5, NULL);
	    gtk_widget_show (label);
	    g_object_set (label, "yalign", 0.5, NULL);
	    break;
	case BUTTON_STYLE_ICON:
	    gtk_widget_show (icon);
	    g_object_set (icon, "yalign", 0.5, NULL);
	    gtk_widget_hide (label);
	    g_object_set (label, "yalign", 0.5, NULL);
	default:
	    break;
	}
    }
}

static void
ctw_button_cb (GtkToggleButton *button, gpointer data)
{
    MtData *mt = data;

    if (gtk_toggle_button_get_active (button)) {
	GSList *group;

	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
	mt_service_set_clicktype (mt->service, 
				  g_slist_index (group, button),
				  NULL);
    }
}

static gboolean
ctw_context_menu (GtkWidget *widget, GdkEventButton *bev, gpointer data)
{
    MtData *mt = data;

    if (bev->button == 3) {
	gtk_menu_popup (GTK_MENU (WID ("popup")),
			0, 0, 0, 0, bev->button, bev->time);
	return TRUE;
    }

    return FALSE;
}

static void
ctw_menu_toggled (GtkCheckMenuItem *item, gpointer data)
{
    MtData *mt = data;
    GSList *group;
    gint index;

    if (!gtk_check_menu_item_get_active (item))
	return;

    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    index = g_slist_index (group, item);
    gconf_client_set_int (mt->client, OPT_STYLE, index, NULL);
}

static gboolean
ctw_delete_cb (GtkWidget *win, GdkEvent *ev, gpointer data)
{
    MtData *mt = data;

    gconf_client_set_bool (mt->client, OPT_CTW, FALSE, NULL);

    return TRUE;
}

GtkWidget *
mt_ctw_get_window (MtData *mt)
{
    return WID ("ctw");
}

gboolean
mt_ctw_init (MtData *mt, gint x, gint y)
{
    GtkWidget *ctw, *w;
    GError *error = NULL;
    const gchar *b[] = { "single", "double", "drag", "right" };
    GSList *group;
    gpointer data;
    gint i;

    mt->ui = gtk_builder_new ();
    gtk_builder_add_from_file (mt->ui, DATADIR "/mousetweaks.ui", &error);
    if (error) {
	g_print ("%s\n", error->message);
	g_error_free (error);

	g_object_unref (mt->ui);
	mt->ui = NULL;

	return FALSE;
    }

    ctw = mt_ctw_get_window (mt);
    gtk_window_stick (GTK_WINDOW (ctw));
    gtk_window_set_keep_above (GTK_WINDOW (ctw), TRUE);
    g_signal_connect (ctw, "delete-event", G_CALLBACK (ctw_delete_cb), mt);

    for (i = 0; i < N_CLICK_TYPES; i++) {
	w = WID (b[i]);
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (w), FALSE);
	g_signal_connect (w, "toggled", G_CALLBACK (ctw_button_cb), mt);
	g_signal_connect (w, "button-press-event",
			  G_CALLBACK (ctw_context_menu), mt);
    }

    g_signal_connect (WID ("text"), "toggled", 
		      G_CALLBACK (ctw_menu_toggled), mt);
    g_signal_connect (WID ("icon"), "toggled", 
		      G_CALLBACK (ctw_menu_toggled), mt);
    w = WID ("both");
    g_signal_connect (w, "toggled", 
		      G_CALLBACK (ctw_menu_toggled), mt);
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (w));
    data = g_slist_nth_data (group, mt->style);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (data), TRUE);

    gtk_widget_realize (ctw);
    mt_ctw_update_style (mt, mt->style);
    mt_ctw_update_sensitivity (mt);
    mt_ctw_update_visibility (mt);

    if (x != -1 && y != -1)
	gtk_window_move (GTK_WINDOW (ctw), x, y);

    return TRUE;
}
