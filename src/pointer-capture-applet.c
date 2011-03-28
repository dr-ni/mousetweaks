/*
 * Copyright © 2007-2010 Gerd Kohlberger <gerdko gmail com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <panel-applet.h>

#include "mt-common.h"

#define PC_TYPE_APPLET  (pc_applet_get_type ())
#define PC_APPLET(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), PC_TYPE_APPLET, PcApplet))
#define PC_IS_APPLET(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), PC_TYPE_APPLET))

#define WID(n) (GTK_WIDGET (gtk_builder_get_object (pc->ui, (n))))

#define PC_MOD_MASK (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)

typedef PanelAppletClass PcAppletClass;
typedef struct _PcApplet
{
    PanelApplet  parent;

    GSettings   *settings;
    GtkBuilder  *ui;
    GtkWidget   *area;
    GtkWidget   *box;
    GtkWidget   *label;
    GtkWidget   *icon;

    GdkCursor   *blank;
    gboolean     pointer_locked;
    gint         pointer_x;
    gint         pointer_y;
    gint         center_x;
    gint         center_y;
    gboolean     horizontal;

    GtkSpinButton   *size;
    GtkSpinButton   *mouse_button;
    GtkToggleButton *modifier_shift;
    GtkToggleButton *modifier_alt;
    GtkToggleButton *modifier_control;
} PcApplet;

static const gchar menu_xml[] =
{
    "<menuitem name=\"item1\" action=\"Preferences\" />"
    "<separator/>"
    "<menuitem name=\"item2\" action=\"Help\" />"
    "<menuitem name=\"item3\" action=\"About\" />"
};

GType pc_applet_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (PcApplet, pc_applet, PANEL_TYPE_APPLET)

static void
pc_applet_init (PcApplet *pc)
{
    AtkObject *atk;

    atk = gtk_widget_get_accessible (GTK_WIDGET (pc));
    if (GTK_IS_ACCESSIBLE (atk))
    {
        atk_object_set_name (atk, _("Capture area"));
        atk_object_set_description (atk, _("Temporarily lock the mouse pointer"));
    }

    pc->settings = g_settings_new ("org.gnome.pointer-capture");
    pc->blank = gdk_cursor_new (GDK_BLANK_CURSOR);
    pc->pointer_locked = FALSE;
    pc->pointer_x = 0;
    pc->pointer_y = 0;
    pc->center_x = 0;
    pc->center_y = 0;
}

static void
pc_applet_change_orient (PanelApplet      *applet,
                         PanelAppletOrient orient)
{
    PcApplet *pc = PC_APPLET (applet);
    gint size;

    pc->horizontal = orient == PANEL_APPLET_ORIENT_UP ||
                     orient == PANEL_APPLET_ORIENT_DOWN;

    if (pc->area || pc->label)
    {
        size = gtk_spin_button_get_value_as_int (pc->size);

        if (pc->horizontal)
        {
            gtk_widget_set_size_request (pc->area, size, -1);
            gtk_label_set_angle (GTK_LABEL (pc->label), 0);
        }
        else
        {
            gtk_widget_set_size_request (pc->area, -1, size);
            gtk_label_set_angle (GTK_LABEL (pc->label), 90);
        }
    }
}

static void
pc_applet_destroy (GtkObject *object)
{
    PcApplet *pc = PC_APPLET (object);

    if (pc->blank)
    {
        gdk_cursor_unref (pc->blank);
        pc->blank = NULL;
    }

    if (pc->ui)
    {
        gtk_widget_destroy (WID ("about"));
        gtk_widget_destroy (WID ("preferences"));
        g_object_unref (pc->ui);
        pc->ui = NULL;
    }

    if (pc->settings)
    {
        g_object_unref (pc->settings);
        pc->settings = NULL;
    }
}

static void
pc_applet_class_init (PcAppletClass *klass)
{
    PanelAppletClass *applet_class;

    applet_class = PANEL_APPLET_CLASS (klass);
    applet_class->change_orient = pc_applet_change_orient;
}

static void
capture_preferences (GtkAction *action, PcApplet *pc)
{
    gtk_window_present (GTK_WINDOW (WID ("preferences")));
}

static void
capture_help (GtkAction *action, PcApplet *pc)
{
    mt_common_show_help (gtk_widget_get_screen (pc->area),
                         gtk_get_current_event_time ());
}

static void
capture_about (GtkAction *action, PcApplet *pc)
{
    gtk_window_present (GTK_WINDOW (WID ("about")));
}

static const GtkActionEntry menu_actions[] =
{
    { "Preferences", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL, NULL,
      G_CALLBACK (capture_preferences) },
    { "Help", GTK_STOCK_HELP, N_("_Help"), NULL, NULL,
      G_CALLBACK (capture_help) },
    { "About", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL,
      G_CALLBACK (capture_about) }
};

static void
lock_pointer (PcApplet *pc)
{
    GdkWindow *win;
    gint x, y, w, h;

    /* set invisible cursor */
    win = gdk_screen_get_root_window (gtk_widget_get_screen (pc->area));
    gdk_window_set_cursor (win, pc->blank);

    /* calculate center position */
    win = gtk_widget_get_window (pc->area);
    gdk_drawable_get_size (win, &w, &h);
    gdk_window_get_origin (win, &x, &y);
    pc->center_x = x + (w >> 1);
    pc->center_y = y + (h >> 1);

    /* update state */
    pc->pointer_locked = TRUE;
    gtk_widget_show (pc->label);
    gtk_widget_hide (pc->icon);
}

static void
unlock_pointer (PcApplet *pc)
{
    GdkWindow *root;
    GdkCursor *cursor;

    /* move pointer to the position where it was locked */
    gdk_display_warp_pointer (gdk_display_get_default (),
                              gtk_widget_get_screen (pc->area),
                              pc->pointer_x,
                              pc->pointer_y);

    /* restore cursor */
    root = gdk_screen_get_root_window (gtk_widget_get_screen (pc->area));
    cursor = gdk_cursor_new (GDK_LEFT_PTR);
    gdk_window_set_cursor (root, cursor);
    gdk_cursor_unref (cursor);

    /* update state */
    pc->pointer_locked = FALSE;
    gtk_widget_hide (pc->label);
    gtk_widget_show (pc->icon);
}

static gboolean
pc_applet_leave_notify (GtkWidget        *widget,
                        GdkEventCrossing *event,
                        PcApplet         *pc)
{
    if (pc->pointer_locked)
    {
        /* move pointer back to center */
        gdk_display_warp_pointer (gdk_display_get_default (),
                                  gtk_widget_get_screen (widget),
                                  pc->center_x,
                                  pc->center_y);
    }
    return FALSE;
}

/* change lock states */
static gboolean
pc_applet_button_press (GtkWidget      *widget,
                        GdkEventButton *event,
                        PcApplet       *pc)
{
    gint button;
    guint mask = 0;

    button = gtk_spin_button_get_value_as_int (pc->mouse_button);

    if (gtk_toggle_button_get_active (pc->modifier_shift))
        mask |= GDK_SHIFT_MASK;
    if (gtk_toggle_button_get_active (pc->modifier_control))
        mask |= GDK_CONTROL_MASK;
    if (gtk_toggle_button_get_active (pc->modifier_alt))
        mask |= GDK_MOD1_MASK;

    if (event->button == button && (event->state & PC_MOD_MASK) == mask)
    {
        if (pc->pointer_locked)
        {
            unlock_pointer (pc);
        }
        else
        {
            pc->pointer_x = event->x_root;
            pc->pointer_y = event->y_root;
            lock_pointer (pc);
        }
        return TRUE;
    }

    return pc->pointer_locked;
}

static gboolean
pc_applet_visibility_notify (GtkWidget          *widget,
                             GdkEventVisibility *event,
                             PcApplet           *pc)
{
    if (event->state != GDK_VISIBILITY_UNOBSCURED && pc->pointer_locked)
    {
        unlock_pointer (pc);
    }
    return FALSE;
}

static gboolean
area_exposed (GtkWidget      *widget,
              GdkEventExpose *event,
              PcApplet       *pc)
{
    GtkAllocation alloc;

    gtk_widget_get_allocation (widget, &alloc);

    if (pc->pointer_locked)
        gtk_paint_flat_box (gtk_widget_get_style (widget),
                            gtk_widget_get_window (widget),
                            GTK_STATE_SELECTED,
                            GTK_SHADOW_NONE,
                            &event->area, widget, NULL,
                            alloc.x, alloc.y, alloc.width, alloc.height);

    gtk_paint_shadow (gtk_widget_get_style (widget),
                      gtk_widget_get_window (widget),
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_ETCHED_IN,
                      &event->area, widget, NULL,
                      alloc.x, alloc.y, alloc.width, alloc.height);
    return FALSE;
}

static void
icon_theme_changed (GtkIconTheme *theme, PcApplet *pc)
{
    gboolean visible;

    visible = gtk_widget_get_visible (pc->icon);
    gtk_container_remove (GTK_CONTAINER (pc->box), pc->icon);
    pc->icon = gtk_image_new_from_icon_name ("input-mouse", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (pc->box), pc->icon);

    if (visible)
        gtk_widget_show (pc->icon);
}

/* about dialog callbacks */
static void
about_response (GtkButton *button, gint response, PcApplet *pc)
{
    gtk_widget_hide (WID ("about"));
}

/* preferences dialog callbacks */
static void
prefs_size_changed (GtkSpinButton *spin, PcApplet *pc)
{
    gint size;

    size = gtk_spin_button_get_value_as_int (pc->size);

    if (pc->horizontal)
    {
        gtk_widget_set_size_request (pc->area, size, -1);
    }
    else
    {
        gtk_widget_set_size_request (pc->area, -1, size);
    }
}

static void
prefs_closed (GtkButton *button, PcApplet *pc)
{
    gtk_widget_hide (WID ("preferences"));
}

static void
prefs_help (GtkButton *button, gpointer data)
{
    mt_common_show_help (gtk_widget_get_screen (GTK_WIDGET (button)),
                         gtk_get_current_event_time ());
}

static gboolean
init_preferences (PcApplet *pc)
{
    GError *error = NULL;

    pc->ui = gtk_builder_new ();
    gtk_builder_add_from_file (pc->ui,
                               DATADIR "/pointer-capture-applet.ui",
                               &error);
    if (error)
    {
        g_print ("%s\n", error->message);
        g_error_free (error);
        return FALSE;
    }

    g_signal_connect (WID ("preferences"), "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (WID ("close"), "clicked",
                      G_CALLBACK (prefs_closed), pc);
    g_signal_connect (WID ("help"), "clicked",
                      G_CALLBACK (prefs_help), NULL);

    /* settings */
    pc->size = GTK_SPIN_BUTTON (WID ("size"));
    g_settings_bind (pc->settings, "size",
                     pc->size, "value",
                     G_SETTINGS_BIND_DEFAULT);

    pc->mouse_button = GTK_SPIN_BUTTON (WID ("mouse_button"));
    g_settings_bind (pc->settings, "mouse-button",
                     pc->mouse_button, "value",
                     G_SETTINGS_BIND_DEFAULT);

    pc->modifier_shift = GTK_TOGGLE_BUTTON (WID ("modifier_shift"));
    g_settings_bind (pc->settings, "modifier-shift",
                     pc->modifier_shift, "active",
                     G_SETTINGS_BIND_DEFAULT);

    pc->modifier_alt = GTK_TOGGLE_BUTTON (WID ("modifier_alt"));
    g_settings_bind (pc->settings, "modifier-alt",
                     pc->modifier_alt, "active",
                     G_SETTINGS_BIND_DEFAULT);

    pc->modifier_control = GTK_TOGGLE_BUTTON (WID ("modifier_control"));
    g_settings_bind (pc->settings, "modifier-control",
                     pc->modifier_control, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_signal_connect (pc->size, "value_changed",
                      G_CALLBACK (prefs_size_changed), pc);

    return TRUE;
}

static gboolean
fill_applet (PanelApplet *applet)
{
    PcApplet *pc = PC_APPLET (applet);
    GtkIconTheme *theme;
    GtkWidget *about;
    GtkActionGroup *group;

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_application_name (_("Pointer Capture Applet"));
    gtk_window_set_default_icon_name (MT_ICON_NAME);

    /* preferences dialog */
    if (!init_preferences (pc))
        return FALSE;

    /* about dialog */
    about = WID ("about");
    g_object_set (about, "version", VERSION, NULL);
    g_signal_connect (about, "response", G_CALLBACK (about_response), pc);
    g_signal_connect (about, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    /* applet setup */
    panel_applet_set_background_widget (applet, GTK_WIDGET (applet));
    panel_applet_set_flags (applet, PANEL_APPLET_EXPAND_MINOR |
                                    PANEL_APPLET_HAS_HANDLE);

    g_signal_connect (pc, "destroy",
                      G_CALLBACK (pc_applet_destroy), NULL);
    g_signal_connect (pc, "leave-notify-event",
                      G_CALLBACK (pc_applet_leave_notify), pc);
    g_signal_connect (pc, "button-press-event",
                      G_CALLBACK (pc_applet_button_press), pc);
    g_signal_connect (pc, "visibility_notify_event",
                      G_CALLBACK (pc_applet_visibility_notify), pc);

    /* context menu */
    group = gtk_action_group_new ("actions");
    gtk_action_group_set_translation_domain (group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (group, menu_actions, 3, pc);
    panel_applet_setup_menu (applet, menu_xml, group);
    g_object_unref (group);

    /* icon theme */
    theme = gtk_icon_theme_get_default ();
    g_signal_connect (theme, "changed", G_CALLBACK (icon_theme_changed), pc);

    /* capture area */
    pc->area = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (pc->area), FALSE);
    gtk_container_add (GTK_CONTAINER (applet), pc->area);
    gtk_widget_show (pc->area);

    g_signal_connect (pc->area, "expose-event", G_CALLBACK (area_exposed), pc);

    pc->box = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (pc->area), pc->box);
    gtk_widget_show (pc->box);

    pc->label = gtk_label_new (_("Locked"));
    gtk_widget_set_state (pc->label, GTK_STATE_SELECTED);
    gtk_container_add (GTK_CONTAINER (pc->box), pc->label);

    pc->icon = gtk_image_new_from_icon_name ("input-mouse", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (pc->box), pc->icon);
    gtk_widget_show (pc->icon);

    pc_applet_change_orient (applet, panel_applet_get_orient (applet));
    gtk_widget_show (GTK_WIDGET (applet));

    return TRUE;
}

static gboolean
applet_factory (PanelApplet *applet, const gchar *iid, gpointer data)
{
    if (!g_str_equal (iid, "PointerCaptureApplet"))
        return FALSE;

    return fill_applet (applet);
}

PANEL_APPLET_OUT_PROCESS_FACTORY ("PointerCaptureAppletFactory",
                                  PC_TYPE_APPLET,
                                  "Pointer Capture Applet",
                                  applet_factory,
                                  NULL)
