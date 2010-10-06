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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <panel-applet.h>
#include <panel-applet-gconf.h>

#include "mt-common.h"

#define PC_TYPE_APPLET  (pc_applet_get_type ())
#define PC_APPLET(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), PC_TYPE_APPLET, PcApplet))
#define PC_IS_APPLET(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), PC_TYPE_APPLET))

#define WID(n) (GTK_WIDGET (gtk_builder_get_object (pc->ui, (n))))

typedef PanelAppletClass PcAppletClass;
typedef struct _PcApplet
{
    PanelApplet  parent;

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

    gint         size;
    gint         cap_button;
    guint        cap_mask;
    gint         rel_button;
    guint        rel_mask;
} PcApplet;

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

    pc->ui = NULL;
    pc->area = NULL;
    pc->box = NULL;
    pc->label = NULL;
    pc->icon = NULL;
    pc->blank = gdk_cursor_new (GDK_BLANK_CURSOR);
    pc->pointer_locked = FALSE;
    pc->pointer_x = 0;
    pc->pointer_y = 0;
    pc->center_x = 0;
    pc->center_y = 0;

    pc->size = 70;
    pc->cap_button = 1;
    pc->cap_mask = 0;
    pc->rel_button = 1;
    pc->rel_mask = 0;
}

static void
pc_applet_change_orient (PanelApplet      *applet,
                         PanelAppletOrient orient)
{
    PcApplet *pc = PC_APPLET (applet);

    if (pc->area || pc->label)
    {
        if (orient == PANEL_APPLET_ORIENT_UP ||
            orient == PANEL_APPLET_ORIENT_DOWN)
        {
            gtk_widget_set_size_request (pc->area, pc->size, -1);
            gtk_label_set_angle (GTK_LABEL (pc->label), 0);
        }
        else
        {
            gtk_widget_set_size_request (pc->area, -1, pc->size);
            gtk_label_set_angle (GTK_LABEL (pc->label), 90);
        }
    }
}

static void
pc_applet_destroy (GtkWidget *widget)
{
    PcApplet *pc = PC_APPLET (widget);

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
}

static void
pc_applet_class_init (PcAppletClass *klass)
{
    GtkWidgetClass *widget_class;
    PanelAppletClass *applet_class;

    widget_class = GTK_WIDGET_CLASS (klass);
    widget_class->destroy = pc_applet_destroy;

    applet_class = PANEL_APPLET_CLASS (klass);
    applet_class->change_orient = pc_applet_change_orient;
}

static void
capture_preferences (BonoboUIComponent *component,
                     PcApplet          *pc,
                     const gchar       *cname)
{
    gtk_window_present (GTK_WINDOW (WID ("preferences")));
}

static void
capture_help (BonoboUIComponent *component,
              PcApplet          *pc,
              const gchar       *cname)
{
    mt_common_show_help (gtk_widget_get_screen (pc->area),
                         gtk_get_current_event_time ());
}

static void
capture_about (BonoboUIComponent *component,
               PcApplet          *pc,
               const gchar       *cname)
{
    gtk_window_present (GTK_WINDOW (WID ("about")));
}

static const BonoboUIVerb menu_verb[] =
{
    BONOBO_UI_UNSAFE_VERB ("PropertiesVerb", capture_preferences),
    BONOBO_UI_UNSAFE_VERB ("HelpVerb", capture_help),
    BONOBO_UI_UNSAFE_VERB ("AboutVerb", capture_about),
    BONOBO_UI_VERB_END
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
pc_applet_enter_notify (GtkWidget        *widget,
                        GdkEventCrossing *event,
                        PcApplet         *pc)
{
    /* lock the pointer immediately if we have no button */
    if (!pc->cap_button && event->mode == GDK_CROSSING_NORMAL)
    {
        pc->pointer_x = event->x_root;
        pc->pointer_y = event->y_root;
        lock_pointer (pc);
    }
    return FALSE;
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
    if (event->button == pc->cap_button &&
        (event->state & pc->cap_mask) == pc->cap_mask &&
        !pc->pointer_locked)
    {
        pc->pointer_x = event->x_root;
        pc->pointer_y = event->y_root;
        lock_pointer (pc);
        return TRUE;
    }
    else if (event->button == pc->rel_button &&
             (event->state & pc->rel_mask) == pc->rel_mask &&
             pc->pointer_locked)
    {
        unlock_pointer (pc);
        return TRUE;
    }
    return FALSE;
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
    PanelApplet *applet = PANEL_APPLET (pc);

    pc->size = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (applet, "size", pc->size, NULL);

    pc_applet_change_orient (applet, panel_applet_get_orient (applet));
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

static void
prefs_cap_button (GtkSpinButton *spin, PcApplet *pc)
{
    pc->cap_button = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (PANEL_APPLET (pc),
                                "capture_button",
                                pc->cap_button,
                                NULL);
}

static void
prefs_cap_alt (GtkToggleButton *toggle, PcApplet *pc)
{
    pc->cap_mask ^= GDK_MOD1_MASK;
    panel_applet_gconf_set_bool (PANEL_APPLET (pc),
                                 "capture_mod_alt",
                                 gtk_toggle_button_get_active (toggle),
                                 NULL);
}

static void
prefs_cap_shift (GtkToggleButton *toggle, PcApplet *pc)
{
    pc->cap_mask ^= GDK_SHIFT_MASK;
    panel_applet_gconf_set_bool (PANEL_APPLET (pc),
                                 "capture_mod_shift",
                                 gtk_toggle_button_get_active (toggle),
                                 NULL);
}

static void
prefs_cap_ctrl (GtkToggleButton *toggle, PcApplet *pc)
{
    pc->cap_mask ^= GDK_CONTROL_MASK;
    panel_applet_gconf_set_bool (PANEL_APPLET (pc),
                                 "capture_mod_ctrl",
                                 gtk_toggle_button_get_active (toggle),
                                 NULL);
}

static void
prefs_rel_button (GtkSpinButton *spin, PcApplet *pc)
{
    pc->rel_button = gtk_spin_button_get_value_as_int (spin);
    panel_applet_gconf_set_int (PANEL_APPLET (pc),
                                "release_button",
                                pc->rel_button,
                                NULL);
}

static void
prefs_rel_alt (GtkToggleButton *toggle, PcApplet *pc)
{
    pc->rel_mask ^= GDK_MOD1_MASK;
    panel_applet_gconf_set_bool (PANEL_APPLET (pc),
                                 "release_mod_alt",
                                 gtk_toggle_button_get_active (toggle),
                                 NULL);
}

static void
prefs_rel_shift (GtkToggleButton *toggle, PcApplet *pc)
{
    pc->rel_mask ^= GDK_SHIFT_MASK;
    panel_applet_gconf_set_bool (PANEL_APPLET (pc),
                                 "release_mod_shift",
                                 gtk_toggle_button_get_active (toggle),
                                 NULL);
}

static void
prefs_rel_ctrl (GtkToggleButton *toggle, PcApplet *pc)
{
    pc->rel_mask ^= GDK_CONTROL_MASK;
    panel_applet_gconf_set_bool (PANEL_APPLET (pc),
                                 "release_mod_ctrl",
                                 gtk_toggle_button_get_active (toggle),
                                 NULL);
}

static gboolean
init_preferences (PcApplet *pc)
{
    GtkWidget *w;
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

    /* size */
    w = WID ("size");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), pc->size);
    g_signal_connect (w, "value_changed",
                      G_CALLBACK (prefs_size_changed), pc);

    /* capture modifier */
    w = WID ("cap_button");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), pc->cap_button);
    g_signal_connect (w, "value_changed", G_CALLBACK (prefs_cap_button), pc);

    w = WID ("cap_alt");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                  pc->cap_mask & GDK_MOD1_MASK);
    g_signal_connect (w, "toggled", G_CALLBACK (prefs_cap_alt), pc);

    w = WID ("cap_shift");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                  pc->cap_mask & GDK_SHIFT_MASK);
    g_signal_connect (w, "toggled", G_CALLBACK (prefs_cap_shift), pc);

    w = WID ("cap_ctrl");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                  pc->cap_mask & GDK_CONTROL_MASK);
    g_signal_connect (w, "toggled", G_CALLBACK (prefs_cap_ctrl), pc);

    /* release modifier */
    w = WID ("rel_button");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), pc->rel_button);
    g_signal_connect (w, "value_changed", G_CALLBACK (prefs_rel_button), pc);

    w = WID ("rel_alt");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                  pc->rel_mask & GDK_MOD1_MASK);
    g_signal_connect (w, "toggled", G_CALLBACK (prefs_rel_alt), pc);

    w = WID ("rel_shift");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                  pc->rel_mask & GDK_SHIFT_MASK);
    g_signal_connect (w, "toggled", G_CALLBACK (prefs_rel_shift), pc);

    w = WID ("rel_ctrl");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                  pc->rel_mask & GDK_CONTROL_MASK);
    g_signal_connect (w, "toggled", G_CALLBACK (prefs_rel_ctrl), pc);

    return TRUE;
}

static gboolean
fill_applet (PanelApplet *applet)
{
    PcApplet *pc = PC_APPLET (applet);
    GtkIconTheme *theme;
    GtkWidget *about;

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_application_name (_("Pointer Capture"));
    gtk_window_set_default_icon_name (MT_ICON_NAME);

    /* gconf settings */
    panel_applet_add_preferences (applet, "/schemas/apps/pointer-capture", NULL);

    pc->size = panel_applet_gconf_get_int (applet, "size", NULL);
    pc->cap_button = panel_applet_gconf_get_int (applet, "capture_button", NULL);
    pc->rel_button = panel_applet_gconf_get_int (applet, "release_button", NULL);

    if (panel_applet_gconf_get_bool (applet, "capture_mod_shift", NULL))
        pc->cap_mask |= GDK_SHIFT_MASK;
    if (panel_applet_gconf_get_bool (applet, "capture_mod_ctrl", NULL))
        pc->cap_mask |= GDK_CONTROL_MASK;
    if (panel_applet_gconf_get_bool (applet, "capture_mod_alt", NULL))
        pc->cap_mask |= GDK_MOD1_MASK;
    if (panel_applet_gconf_get_bool (applet, "release_mod_shift", NULL))
        pc->rel_mask |= GDK_SHIFT_MASK;
    if (panel_applet_gconf_get_bool (applet, "release_mod_ctrl", NULL))
        pc->rel_mask |= GDK_CONTROL_MASK;
    if (panel_applet_gconf_get_bool (applet, "release_mod_alt", NULL))
        pc->rel_mask |= GDK_MOD1_MASK;

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
    panel_applet_setup_menu_from_file (applet, DATADIR, "PointerCapture.xml",
                                       NULL, menu_verb, pc);

    g_signal_connect (pc, "enter-notify-event",
                      G_CALLBACK (pc_applet_enter_notify), pc);
    g_signal_connect (pc, "leave-notify-event",
                      G_CALLBACK (pc_applet_leave_notify), pc);
    g_signal_connect (pc, "button-press-event",
                      G_CALLBACK (pc_applet_button_press), pc);
    g_signal_connect (pc, "visibility_notify_event",
                      G_CALLBACK (pc_applet_visibility_notify), pc);

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
    if (!g_str_equal (iid, "OAFIID:PointerCaptureApplet"))
        return FALSE;

    return fill_applet (applet);
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:PointerCaptureApplet_Factory",
                             PC_TYPE_APPLET,
                             "Pointer Capture Factory",
                             VERSION,
                             applet_factory,
                             NULL);
