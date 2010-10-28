/*
 * Copyright © 2010 Gerd Kohlberger <gerdko gmail com>
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

#ifndef __MT_SETTINGS_H__
#define __MT_SETTINGS_H__

#include <gio/gio.h>

#include "mt-common.h"

G_BEGIN_DECLS

#define MT_TYPE_SETTINGS  (mt_settings_get_type ())
#define MT_SETTINGS(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_SETTINGS, MtSettings))
#define MT_IS_SETTINGS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_SETTINGS))

typedef GObjectClass MtSettingsClass;
typedef struct _MtSettings MtSettings;

struct _MtSettings
{
    GObject                         parent;

    GSettings                      *mt_settings;
    GSettings                      *a11y_settings;

    gint                            dwell_threshold;
    GDesktopMouseDwellMode          dwell_mode;
    GDesktopMouseDwellDirection     dwell_gesture_single;
    GDesktopMouseDwellDirection     dwell_gesture_double;
    GDesktopMouseDwellDirection     dwell_gesture_drag;
    GDesktopMouseDwellDirection     dwell_gesture_secondary;

    MtClickTypeWindowStyle          ctw_style;
    MtClickTypeWindowOrientation    ctw_orientation;
    gchar                          *ctw_geometry;

    guint                           ctw_visible    : 1;
    guint                           dwell_enabled  : 1;
    guint                           ssc_enabled    : 1;
};

GType             mt_settings_get_type              (void) G_GNUC_CONST;
MtSettings *      mt_settings_get_default           (void);

G_END_DECLS

#endif /* __MT_SETTINGS_H__ */
