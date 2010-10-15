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

#include <gtk/gtk.h>

#include "mt-ctw.h"
#include "mt-service.h"
#include "mt-settings.h"
#include "mt-common.h"

#define O(n) (gtk_builder_get_object (builder, (n)))
#define W(n) (GTK_WIDGET (O(n)))

static GtkBuilder *builder;

enum
{
    BUTTON_STYLE_TEXT = 0,
    BUTTON_STYLE_ICON,
    BUTTON_STYLE_BOTH
};

static void
service_click_type_changed (MtService *service, GParamSpec *pspec)
{
    GSList *group;
    GtkWidget *button;

    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (O ("single")));
    button = g_slist_nth_data (group, mt_service_get_click_type (service));
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
                              ms->dwell_mode == DWELL_MODE_CTW);
}

static void
ctw_style_changed (MtSettings *ms, GParamSpec *pspec)
{
    GtkWidget *icon, *label;
    const gchar *l[] = { "single_l", "double_l", "drag_l", "right_l" };
    const gchar *img[] = { "single_i", "double_i", "drag_i", "right_i" };
    gint i;

    for (i = 0; i < N_CLICK_TYPES; i++)
    {
        label = W (l[i]);
        icon = W (img[i]);

        switch (ms->ctw_style)
        {
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
        gtk_menu_popup (GTK_MENU (O ("popup")), 0, 0, 0, 0, bev->button, bev->time);
        return TRUE;
    }
    return FALSE;
}

static void
ctw_menu_toggled (GtkCheckMenuItem *item, gpointer data)
{
    GSList *group;

    if (gtk_check_menu_item_get_active (item))
    {
        group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
        g_object_set (mt_settings_get_default (),
                      "ctw-style", g_slist_index (group, item),
                      NULL);
    }
}

static gboolean
ctw_delete_cb (GtkWidget *win, GdkEvent *ev, gpointer data)
{
    g_object_set (mt_settings_get_default (), "ctw-visible", FALSE, NULL);
    return TRUE;
}

gboolean
mt_ctw_init (gint x, gint y)
{
    MtSettings *ms;
    MtService *service;
    GtkWidget *ctw;
    GObject *obj;
    GError *error = NULL;
    const gchar *b[] = { "single", "double", "drag", "right" };
    GSList *group;
    gpointer data;
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
    g_signal_connect (ctw, "delete-event", G_CALLBACK (ctw_delete_cb), NULL);

    /* init buttons */
    for (i = 0; i < N_CLICK_TYPES; i++)
    {
        obj = O (b[i]);
        gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (obj), FALSE);
        g_signal_connect (obj, "toggled", G_CALLBACK (ctw_button_cb), NULL);
        g_signal_connect (obj, "button-press-event", G_CALLBACK (ctw_context_menu), NULL);
    }

    /* service */
    service = mt_service_get_default ();
    g_signal_connect (service, "notify::click-type",
                      G_CALLBACK (service_click_type_changed), NULL);

    service_click_type_changed (service, NULL);

    /* settings */
    ms = mt_settings_get_default ();
    g_signal_connect (ms, "notify::" KEY_CTW_VISIBLE,
                      G_CALLBACK (ctw_visibility_changed), NULL);
    g_signal_connect (ms, "notify::" KEY_DWELL_ENABLED,
                      G_CALLBACK (ctw_visibility_changed), NULL);
    g_signal_connect (ms, "notify::" KEY_DWELL_MODE,
                      G_CALLBACK (ctw_sensitivity_changed), NULL);
    g_signal_connect (ms, "notify::" KEY_CTW_STYLE,
                      G_CALLBACK (ctw_style_changed), NULL);

    ctw_visibility_changed (ms, NULL);
    ctw_sensitivity_changed (ms, NULL);

    /* init context menu */
    obj = O ("both");
    g_signal_connect (obj, "toggled", G_CALLBACK (ctw_menu_toggled), NULL);
    g_signal_connect (O ("text"), "toggled",  G_CALLBACK (ctw_menu_toggled), NULL);
    g_signal_connect (O ("icon"), "toggled",  G_CALLBACK (ctw_menu_toggled), NULL);

    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (obj));
    data = g_slist_nth_data (group, ms->ctw_style);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (data), TRUE);

    /* XXX: remember window position */
    if (x != -1 && y != -1)
    {
        gtk_widget_realize (ctw);
        gtk_window_move (GTK_WINDOW (ctw), x, y);
    }

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
