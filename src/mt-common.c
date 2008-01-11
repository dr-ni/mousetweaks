/*
 * Copyright © 2007 Gerd Kohlberger <lowfi@chello.at>
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

gint
mt_show_dialog (const gchar *primary,
		const gchar *secondary,
		GtkMessageType type)
{
    GtkWidget *dialog;
    gint ret;

    dialog = gtk_message_dialog_new (NULL,
				     GTK_DIALOG_MODAL,
				     type,
				     GTK_BUTTONS_NONE,
				     primary);
    gtk_window_set_title (GTK_WINDOW(dialog), "Mousetweaks");
    gtk_window_set_icon_name (GTK_WINDOW(dialog), MT_ICON_NAME);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
					      secondary);

    if (type == GTK_MESSAGE_QUESTION)
	gtk_dialog_add_buttons (GTK_DIALOG(dialog),
				GTK_STOCK_YES, GTK_RESPONSE_YES,
				GTK_STOCK_NO, GTK_RESPONSE_NO,
				NULL);
    else
	gtk_dialog_add_button (GTK_DIALOG(dialog),
			       GTK_STOCK_OK, GTK_RESPONSE_OK);

    gtk_widget_show_all (dialog);
    ret = gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);

    return ret;
}
