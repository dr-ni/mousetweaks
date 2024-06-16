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

#include "mt-ctw.h"
#include "mt-service.h"
#include "mt-settings.h"
#include "mt-common.h"

#define O(n) (gtk_builder_get_object (builder, (n)))
#define W(n) (GTK_WIDGET (O(n)))

static GtkBuilder *builder;

static void
service_click_type_changed (MtService *service, GParamSpec *pspec)
{
    GSList *group;
    GtkWidget *button;
    MtDwellClickType ct;

    ct = mt_service_get_click_type (service);

    if (ct == MT_DWELL_CLICK_TYPE_MIDDLE)
        return;

    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (O ("single")));
    button = g_slist_nth_data (group, ct);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    gtk_widget_grab_focus (button);
}

static void
ctw_visibility_changed (MtSettings *ms, GParamSpec *pspec)
{
    gtk_widget_set_visible (mt_ctw_get_window (),
                            ms->dwell_enabled && ms->ctw_visible);
}

static void
ctw_sensitivity_changed (MtSettings *ms, GParamSpec *pspec)
{
    gtk_widget_set_sensitive (mt_ctw_get_window (),
                              ms->dwell_mode == G_DESKTOP_MOUSE_DWELL_MODE_WINDOW);
}

static void
ctw_style_changed (MtSettings *ms, GParamSpec *pspec)
{
    GtkWidget *icon, *label;
    const gchar *label_names[] = { "single_l", "double_l", "drag_l", "right_l" };
    const gchar *icon_names[] = { "single_i", "double_i", "drag_i", "right_i" };
    gint i;

    for (i = 0; i < 4; i++)
    {
        label = W (label_names[i]);
        icon = W (icon_names[i]);

        switch (ms->ctw_style)
        {
            case MT_CLICK_TYPE_WINDOW_STYLE_BOTH:
                g_object_set (icon, "yalign", 1.0, NULL);
                gtk_widget_show (icon);
                g_object_set (label, "yalign", 0.0, NULL);
                gtk_widget_show (label);
                break;
            case MT_CLICK_TYPE_WINDOW_STYLE_TEXT:
                gtk_widget_hide (icon);
                g_object_set (icon, "yalign", 0.5, NULL);
                gtk_widget_show (label);
                g_object_set (label, "yalign", 0.5, NULL);
                break;
            case MT_CLICK_TYPE_WINDOW_STYLE_ICON:
                gtk_widget_show (icon);
                g_object_set (icon, "yalign", 0.5, NULL);
                gtk_widget_hide (label);
                g_object_set (label, "yalign", 0.5, NULL);
                break;
        }
    }
}

static void
ctw_orientation_changed (MtSettings *ms, GParamSpec *pspec)
{
    if (ms->ctw_orientation == MT_CLICK_TYPE_WINDOW_ORIENTATION_HORIZONTAL)
    {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (O ("box")),
                                        GTK_ORIENTATION_HORIZONTAL);
    }
    else
    {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (O ("box")),
                                        GTK_ORIENTATION_VERTICAL);
    }
}

static void
ctw_button_toggled (GtkToggleButton *button, gpointer data)
{
    GSList *group;

    if (gtk_toggle_button_get_active (button))
    {
        group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
        mt_service_set_click_type (mt_service_get_default (),
                                   g_slist_index (group, button));
    }
}

static gboolean
ctw_context_menu (GtkWidget *widget, GdkEventButton *bev, gpointer data)
{
    if (bev->button == 3)
    {
        gtk_menu_popup (GTK_MENU (O ("popup")),
                        0, 0, 0, 0,
                        bev->button, bev->time);
        return TRUE;
    }
    return FALSE;
}

static void
ctw_button_style_icon_toggled (GtkCheckMenuItem *item, gpointer data)
{
    if (gtk_check_menu_item_get_active (item))
    {
        g_object_set (mt_settings_get_default (), "ctw-style",
                      MT_CLICK_TYPE_WINDOW_STYLE_ICON, NULL);
    }
}

static void
ctw_button_style_text_toggled (GtkCheckMenuItem *item, gpointer data)
{
    if (gtk_check_menu_item_get_active (item))
    {
        g_object_set (mt_settings_get_default (), "ctw-style",
                      MT_CLICK_TYPE_WINDOW_STYLE_TEXT, NULL);
    }
}

static void
ctw_button_style_both_toggled (GtkCheckMenuItem *item, gpointer data)
{
    if (gtk_check_menu_item_get_active (item))
    {
        g_object_set (mt_settings_get_default (), "ctw-style",
                      MT_CLICK_TYPE_WINDOW_STYLE_BOTH, NULL);
    }
}

static void
ctw_orientation_horizontal_toggled (GtkCheckMenuItem *item, gpointer data)
{
    if (gtk_check_menu_item_get_active (item))
    {
        g_object_set (mt_settings_get_default (), "ctw-orientation",
                      MT_CLICK_TYPE_WINDOW_ORIENTATION_HORIZONTAL, NULL);
    }
}

static void
ctw_orientation_vertical_toggled (GtkCheckMenuItem *item, gpointer data)
{
    if (gtk_check_menu_item_get_active (item))
    {
        g_object_set (mt_settings_get_default (), "ctw-orientation",
                      MT_CLICK_TYPE_WINDOW_ORIENTATION_VERTICAL, NULL);
    }
}

static void
ctw_menu_label_set_bold (GtkWidget *item)
{
    GtkWidget *label;
    PangoAttrList *list;
    PangoAttribute *attr;

    list = pango_attr_list_new ();
    attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
    pango_attr_list_insert (list, attr);

    label = gtk_bin_get_child (GTK_BIN (item));
    gtk_label_set_attributes (GTK_LABEL (label), list);
    pango_attr_list_unref (list);
}

gboolean
mt_ctw_init (void)
{
    MtSettings *ms;
    MtService *service;
    GtkWidget *ctw;
    GObject *obj;
    GError *error = NULL;
    const gchar *button_names[] = { "single", "double", "drag", "right" };
    gint i;

    /* load UI */
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, DATADIR "/mousetweaks.ui", &error);
    if (error)
    {
        g_print ("%s\n", error->message);
        g_error_free (error);
        g_object_unref (builder);
        return FALSE;
    }

    /* init window */
    ctw = mt_ctw_get_window ();
    gtk_window_stick (GTK_WINDOW (ctw));
    gtk_window_set_keep_above (GTK_WINDOW (ctw), TRUE);

    /* init buttons */
    for (i = 0; i < 4; i++)
    {
        obj = O (button_names[i]);
        gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (obj), FALSE);
        g_object_connect (obj,
                          "signal::toggled", G_CALLBACK (ctw_button_toggled), NULL,
                          "signal::button-press-event", G_CALLBACK (ctw_context_menu), NULL,
                          NULL);
    }

    /* service */
    service = mt_service_get_default ();
    g_signal_connect (service, "notify::click-type",
                      G_CALLBACK (service_click_type_changed), NULL);

    service_click_type_changed (service, NULL);

    /* settings */
    ms = mt_settings_get_default ();
    g_object_connect (ms,
                      "signal::notify::ctw-visible", G_CALLBACK (ctw_visibility_changed), NULL,
                      "signal::notify::ctw-style", G_CALLBACK (ctw_style_changed), NULL,
                      "signal::notify::ctw-orientation", G_CALLBACK (ctw_orientation_changed), NULL,
                      "signal::notify::dwell-enabled", G_CALLBACK (ctw_visibility_changed), NULL,
                      "signal::notify::dwell-mode", G_CALLBACK (ctw_sensitivity_changed), NULL,
                      NULL);

    /* context menu: button style */
    switch (ms->ctw_style)
    {
        case MT_CLICK_TYPE_WINDOW_STYLE_BOTH:
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (O ("menu_both")), TRUE);
            break;
        case MT_CLICK_TYPE_WINDOW_STYLE_TEXT:
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (O ("menu_text")), TRUE);
            break;
        case MT_CLICK_TYPE_WINDOW_STYLE_ICON:
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (O ("menu_icon")), TRUE);
            break;
    }

    g_signal_connect (O ("menu_both"), "toggled",
                      G_CALLBACK (ctw_button_style_both_toggled), NULL);
    g_signal_connect (O ("menu_text"), "toggled",
                      G_CALLBACK (ctw_button_style_text_toggled), NULL);
    g_signal_connect (O ("menu_icon"), "toggled",
                      G_CALLBACK (ctw_button_style_icon_toggled), NULL);

    ctw_style_changed (ms, NULL);
    ctw_menu_label_set_bold (W ("menu_orientation"));

    /* context menu: orientation */
    switch (ms->ctw_orientation)
    {
        case MT_CLICK_TYPE_WINDOW_ORIENTATION_HORIZONTAL:
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (O ("menu_horizontal")), TRUE);
            break;
        case MT_CLICK_TYPE_WINDOW_ORIENTATION_VERTICAL:
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (O ("menu_vertical")), TRUE);
            break;
    }

    g_signal_connect (O ("menu_horizontal"), "toggled",
                      G_CALLBACK (ctw_orientation_horizontal_toggled), NULL);
    g_signal_connect (O ("menu_vertical"), "toggled",
                      G_CALLBACK (ctw_orientation_vertical_toggled), NULL);

    ctw_orientation_changed (ms, NULL);
    ctw_menu_label_set_bold (W ("menu_button_style"));

    /* window geometry */
    gtk_widget_show (W ("box"));
    gtk_window_parse_geometry (GTK_WINDOW (ctw), ms->ctw_geometry);

    ctw_visibility_changed (ms, NULL);
    ctw_sensitivity_changed (ms, NULL);

    return TRUE;
}

void
mt_ctw_fini (void)
{
    if (builder)
    {
        gtk_widget_destroy (mt_ctw_get_window ());
        g_object_unref (builder);
    }
}

GtkWidget *
mt_ctw_get_window (void)
{
    return W ("ctw");
}

void
mt_ctw_save_geometry (void)
{
    GtkWidget *ctw;
    gchar *geometry;
    gint x, y, width, height;

    ctw = mt_ctw_get_window ();

    if (gtk_widget_get_visible (ctw))
    {
        gtk_window_get_size (GTK_WINDOW (ctw), &width, &height);
        gtk_window_get_position (GTK_WINDOW (ctw), &x, &y);

        /* X geometry string */
        geometry = g_strdup_printf ("%ix%i+%i+%i", width, height, x, y);
        g_object_set (mt_settings_get_default (), "ctw-geometry", geometry, NULL);
        g_free (geometry);
    }
}
