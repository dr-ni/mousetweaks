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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mt-common.h"

Display *
mt_common_get_xdisplay (void)
{
    return GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
}

void
mt_common_xtrap_push (void)
{
    gdk_error_trap_push ();
}

void
mt_common_xtrap_pop (void)
{
    gint xerror;

    gdk_flush ();
    if ((xerror = gdk_error_trap_pop ()))
    {
        g_warning ("Trapped X Error code %i.", xerror);
    }
}

GdkScreen *
mt_common_get_screen (void)
{
    GdkDisplay *gdk_dpy;
    GdkScreen *screen;
    gint n_screens;

    gdk_dpy = gdk_display_get_default ();
    n_screens = gdk_display_get_n_screens (gdk_dpy);

    if (n_screens > 1)
    {
        gdk_display_get_pointer (gdk_dpy, &screen, NULL, NULL, NULL);
    }
    else
    {
        screen = gdk_screen_get_default ();
    }
    return screen;
}

void
mt_common_show_help (GdkScreen *screen, guint32 timestamp)
{
    GError *error = NULL;

    if (!gtk_show_uri (screen, "ghelp:mousetweaks", timestamp, &error))
    {
        mt_common_show_dialog (_("Failed to Display Help"),
                               error->message,
                               MT_MESSAGE_TYPE_WARNING);
        g_error_free (error);
    }
}

void
mt_common_show_dialog (const gchar  *primary,
                       const gchar  *secondary,
                       MtMessageType type)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL, 0,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_NONE,
                                     "%s", primary);

    gtk_window_set_title (GTK_WINDOW (dialog), g_get_application_name ());
    gtk_window_set_icon_name (GTK_WINDOW (dialog), MT_ICON_NAME);
    gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary);

    switch (type)
    {
        case MT_MESSAGE_TYPE_WARNING:
            g_object_set (dialog, "message-type", GTK_MESSAGE_WARNING, NULL);
            gtk_dialog_add_button (GTK_DIALOG (dialog),
                                   GTK_STOCK_OK, GTK_RESPONSE_OK);
            break;
        case MT_MESSAGE_TYPE_ERROR:
        default:
            gtk_dialog_add_button (GTK_DIALOG (dialog),
                                   GTK_STOCK_OK, GTK_RESPONSE_OK);
    }

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}
