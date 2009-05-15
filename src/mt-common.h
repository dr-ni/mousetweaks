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

#ifndef __MT_COMMON_H__
#define __MT_COMMON_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

G_BEGIN_DECLS

#define MT_ICON_NAME  "input-mouse"

#define MT_GCONF_HOME "/desktop/gnome/accessibility/mouse"
#define OPT_THRESHOLD MT_GCONF_HOME "/threshold"
#define OPT_DELAY     MT_GCONF_HOME "/delay_enable"
#define OPT_DELAY_T   MT_GCONF_HOME "/delay_time"
#define OPT_DWELL     MT_GCONF_HOME "/dwell_enable"
#define OPT_DWELL_T   MT_GCONF_HOME "/dwell_time"
#define OPT_CTW       MT_GCONF_HOME "/dwell_show_ctw"
#define OPT_MODE      MT_GCONF_HOME "/dwell_mode"
#define OPT_G_SINGLE  MT_GCONF_HOME "/dwell_gesture_single"
#define OPT_G_DOUBLE  MT_GCONF_HOME "/dwell_gesture_double"
#define OPT_G_DRAG    MT_GCONF_HOME "/dwell_gesture_drag"
#define OPT_G_RIGHT   MT_GCONF_HOME "/dwell_gesture_secondary"
#define OPT_STYLE     MT_GCONF_HOME "/button_layout"
#define OPT_ANIMATE   MT_GCONF_HOME "/animate_cursor"

#define GNOME_MOUSE_DIR "/desktop/gnome/peripherals/mouse"
#define GNOME_MOUSE_ORIENT GNOME_MOUSE_DIR "/left_handed"

#define GNOME_A11Y_KEY "/desktop/gnome/interface/accessibility"

enum {
    DWELL_MODE_CTW = 0,
    DWELL_MODE_GESTURE
};

enum {
    DWELL_CLICK_TYPE_RIGHT = 0,
    DWELL_CLICK_TYPE_DRAG,
    DWELL_CLICK_TYPE_DOUBLE,
    DWELL_CLICK_TYPE_SINGLE,
    N_CLICK_TYPES
};

enum {
    DIRECTION_LEFT = 0,
    DIRECTION_RIGHT,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_DISABLE
};

typedef enum {
    MT_MESSAGE_ERROR = 0,
    MT_MESSAGE_WARNING,
    MT_MESSAGE_QUESTION,
    MT_MESSAGE_LOGOUT
} MtMessageType;

void mt_common_show_help   (GdkScreen     *screen,
			    guint32        timestamp);
gint mt_common_show_dialog (const gchar   *primary,
			    const gchar   *secondary,
			    MtMessageType  type);

G_END_DECLS

#endif /* __MT_COMMON_H__ */
