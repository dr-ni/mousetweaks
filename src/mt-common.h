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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gsettings-desktop-schemas/gdesktop-enums.h>

#include "mt-enum-types.h"

G_BEGIN_DECLS

#define MT_ICON_NAME                "input-mouse"

#define MOUSETWEAKS_DBUS_NAME       "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_IFACE      "org.gnome.Mousetweaks"
#define MOUSETWEAKS_DBUS_PATH       "/org/gnome/Mousetweaks"

/* GSettings */
#define MOUSETWEAKS_SCHEMA_ID       "org.gnome.mousetweaks"
#define KEY_CTW_STYLE               "click-type-window-style"
#define KEY_CTW_ORIENTATION         "click-type-window-orientation"
#define KEY_CTW_GEOMETRY            "click-type-window-geometry"

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

typedef enum
{
    MT_DWELL_CLICK_TYPE_RIGHT,
    MT_DWELL_CLICK_TYPE_DRAG,
    MT_DWELL_CLICK_TYPE_DOUBLE,
    MT_DWELL_CLICK_TYPE_SINGLE,
    MT_DWELL_CLICK_TYPE_MIDDLE
} MtDwellClickType;

typedef enum
{
    MT_CLICK_TYPE_WINDOW_STYLE_TEXT,
    MT_CLICK_TYPE_WINDOW_STYLE_ICON,
    MT_CLICK_TYPE_WINDOW_STYLE_BOTH
} MtClickTypeWindowStyle;

typedef enum
{
    MT_CLICK_TYPE_WINDOW_ORIENTATION_HORIZONTAL,
    MT_CLICK_TYPE_WINDOW_ORIENTATION_VERTICAL
} MtClickTypeWindowOrientation;

typedef enum /*< skip >*/
{
    MT_MESSAGE_TYPE_ERROR,
    MT_MESSAGE_TYPE_WARNING,
} MtMessageType;

Display *    mt_common_get_xdisplay       (void);

void         mt_common_xtrap_push         (void);
void         mt_common_xtrap_pop          (void);

GdkDevice *  mt_common_get_client_pointer (void);

GdkScreen *  mt_common_get_screen         (void);

void         mt_common_show_help          (GdkScreen     *screen,
                                           guint32        timestamp);

void         mt_common_show_dialog        (const gchar   *primary,
                                           const gchar   *secondary,
                                           MtMessageType  type);

G_END_DECLS

#endif /* __MT_COMMON_H__ */
