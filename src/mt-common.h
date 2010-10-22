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

#ifndef __MT_COMMON_H__
#define __MT_COMMON_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gsettings-desktop-schemas/gdesktop-enums.h>

G_BEGIN_DECLS

#define MT_ICON_NAME                "input-mouse"

#define MOUSETWEAKS_DBUS_NAME       "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_IFACE      "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_PATH       "/org/gnome/Mousetweaks"

/* GSettings */
#define MOUSETWEAKS_SCHEMA_ID       "org.gnome.mousetweaks"
#define KEY_CTW_STYLE               "click-type-window-style"
#define KEY_ANIMATE_CURSOR          "animate-cursor"

#define A11Y_MOUSE_SCHEMA_ID        "org.gnome.desktop.a11y.mouse"
#define KEY_DWELL_ENABLED           "dwell-click-enabled"
#define KEY_DWELL_TIME              "dwell-time"
#define KEY_DWELL_THRESHOLD         "dwell-threshold"
#define KEY_DWELL_MODE              "dwell-mode"
#define KEY_DWELL_GESTURE_SINGLE    "dwell-gesture-single"
#define KEY_DWELL_GESTURE_DOUBLE    "dwell-gesture-double"
#define KEY_DWELL_GESTURE_DRAG      "dwell-gesture-drag"
#define KEY_DWELL_GESTURE_SECONDARY "dwell-gesture-secondary"
#define KEY_SSC_ENABLED             "secondary-click-enabled"
#define KEY_SSC_TIME                "secondary-click-time"
#define KEY_CTW_VISIBLE             "click-type-window-visible"

#define G_DESKTOP_TYPE_MOUSE_DWELL_MODE      (g_desktop_mouse_dwell_mode_get_type())
#define G_DESKTOP_TYPE_MOUSE_DWELL_DIRECTION (g_desktop_mouse_dwell_direction_get_type())

GType   g_desktop_mouse_dwell_mode_get_type      (void) G_GNUC_CONST;
GType   g_desktop_mouse_dwell_direction_get_type (void) G_GNUC_CONST;

typedef enum
{
    DWELL_CLICK_TYPE_RIGHT = 0,
    DWELL_CLICK_TYPE_DRAG,
    DWELL_CLICK_TYPE_DOUBLE,
    DWELL_CLICK_TYPE_SINGLE,
    N_CLICK_TYPES
} MtClickType;

typedef enum
{
    MT_MESSAGE_ERROR = 0,
    MT_MESSAGE_WARNING,
    MT_MESSAGE_QUESTION,
    MT_MESSAGE_LOGOUT
} MtMessageType;

Display *    mt_common_get_xdisplay      (void);

void         mt_common_xtrap_push        (void);
void         mt_common_xtrap_pop         (void);

GdkScreen *  mt_common_get_screen        (void);

void         mt_common_show_help         (GdkScreen     *screen,
                                          guint32        timestamp);

gint         mt_common_show_dialog       (const gchar   *primary,
                                          const gchar   *secondary,
                                          MtMessageType  type);

G_END_DECLS

#endif /* __MT_COMMON_H__ */
