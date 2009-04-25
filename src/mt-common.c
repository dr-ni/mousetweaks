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

#include "mt-common.h"

void
mt_common_show_help (GdkScreen *screen, guint32 timestamp)
{
    GError *error = NULL;

    if (!gtk_show_uri (screen, "ghelp:mousetweaks", timestamp, &error)) {
	mt_common_show_dialog (_("Failed to Display Help"),
			       error->message, MT_MESSAGE_WARNING);
	g_error_free (error);
    }
}

gint
mt_common_show_dialog (const gchar  *primary,
		       const gchar  *secondary,
		       MtMessageType type)
{
    GtkWidget *dialog;
    gint ret;

    dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				     GTK_BUTTONS_NONE, "%s", primary);
    gtk_window_set_title (GTK_WINDOW (dialog), g_get_application_name ());
    gtk_window_set_icon_name (GTK_WINDOW (dialog), MT_ICON_NAME);
    gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
					      "%s", secondary);
    switch (type) {
    case MT_MESSAGE_QUESTION:
	g_object_set (dialog, "message-type", GTK_MESSAGE_QUESTION, NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_YES, GTK_RESPONSE_YES,
				GTK_STOCK_NO, GTK_RESPONSE_NO,
				NULL);
	break;
    case MT_MESSAGE_WARNING:
	g_object_set (dialog, "message-type", GTK_MESSAGE_WARNING, NULL);
	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       GTK_STOCK_OK, GTK_RESPONSE_OK);
	break;
    case MT_MESSAGE_LOGOUT:
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				_("Enable and Log Out"), GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_ACCEPT);
	break;
    case MT_MESSAGE_ERROR:
    default:
	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       GTK_STOCK_OK, GTK_RESPONSE_OK);
    }

    ret = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return ret;
}
