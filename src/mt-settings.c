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

#include "mt-settings.h"

#define PFLAGS (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

#define BIND_PROP_RW(s,p,k) (g_settings_bind ((s), (k), ms, (p), \
                             G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY))
#define BIND_PROP(s,p,k)    (g_settings_bind ((s), (k), ms, (p), \
                             G_SETTINGS_BIND_GET | G_SETTINGS_BIND_NO_SENSITIVITY))
enum
{
    PROP_0,
    PROP_DWELL_ENABLED,
    PROP_DWELL_THRESHOLD,
    PROP_DWELL_MODE,
    PROP_DWELL_GESTURE_SINGLE,
    PROP_DWELL_GESTURE_DOUBLE,
    PROP_DWELL_GESTURE_DRAG,
    PROP_DWELL_GESTURE_SECONDARY,
    PROP_SSC_ENABLED,
    PROP_CTW_VISIBLE,
    PROP_CTW_STYLE,
    PROP_CTW_ORIENTATION,
    PROP_CTW_GEOMETRY
};

G_DEFINE_TYPE (MtSettings, mt_settings, G_TYPE_OBJECT)

static void
mt_settings_init (MtSettings *ms)
{
    ms->mt_settings = g_settings_new (MOUSETWEAKS_SCHEMA_ID);
    ms->ctw_geometry = NULL;

    BIND_PROP_RW (ms->mt_settings, "ctw-style", KEY_CTW_STYLE);
    BIND_PROP_RW (ms->mt_settings, "ctw-orientation", KEY_CTW_ORIENTATION);
    BIND_PROP_RW (ms->mt_settings, "ctw-geometry", KEY_CTW_GEOMETRY);

    ms->a11y_settings = g_settings_new (A11Y_MOUSE_SCHEMA_ID);

    BIND_PROP (ms->a11y_settings, "dwell-enabled", KEY_DWELL_ENABLED);
    BIND_PROP (ms->a11y_settings, "dwell-threshold", KEY_DWELL_THRESHOLD);
    BIND_PROP (ms->a11y_settings, "dwell-mode", KEY_DWELL_MODE);
    BIND_PROP (ms->a11y_settings, "dwell-gesture-single", KEY_DWELL_GESTURE_SINGLE);
    BIND_PROP (ms->a11y_settings, "dwell-gesture-double", KEY_DWELL_GESTURE_DOUBLE);
    BIND_PROP (ms->a11y_settings, "dwell-gesture-drag", KEY_DWELL_GESTURE_DRAG);
    BIND_PROP (ms->a11y_settings, "dwell-gesture-secondary", KEY_DWELL_GESTURE_SECONDARY);
    BIND_PROP (ms->a11y_settings, "ssc-enabled", KEY_SSC_ENABLED);
    BIND_PROP (ms->a11y_settings, "ctw-visible", KEY_CTW_VISIBLE);
}

static void
mt_settings_dispose (GObject *object)
{
    MtSettings *ms = MT_SETTINGS (object);

    g_clear_object (&ms->mt_settings);
    g_clear_object (&ms->a11y_settings);

    G_OBJECT_CLASS (mt_settings_parent_class)->dispose (object);
}

static void
mt_settings_finalize (GObject *object)
{
    MtSettings *ms = MT_SETTINGS (object);

    g_free (ms->ctw_geometry);

    G_OBJECT_CLASS (mt_settings_parent_class)->finalize (object);
}

static void
mt_settings_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    MtSettings *ms = MT_SETTINGS (object);

    switch (prop_id)
    {
        case PROP_DWELL_ENABLED:
            ms->dwell_enabled = g_value_get_boolean (value);
            break;
        case PROP_DWELL_THRESHOLD:
            ms->dwell_threshold = g_value_get_int (value);
            break;
        case PROP_DWELL_MODE:
            ms->dwell_mode = g_value_get_enum (value);
            break;
        case PROP_DWELL_GESTURE_SINGLE:
            ms->dwell_gesture_single = g_value_get_enum (value);
            break;
        case PROP_DWELL_GESTURE_DOUBLE:
            ms->dwell_gesture_double = g_value_get_enum (value);
            break;
        case PROP_DWELL_GESTURE_DRAG:
            ms->dwell_gesture_drag = g_value_get_enum (value);
            break;
        case PROP_DWELL_GESTURE_SECONDARY:
            ms->dwell_gesture_secondary = g_value_get_enum (value);
            break;
        case PROP_SSC_ENABLED:
            ms->ssc_enabled = g_value_get_boolean (value);
            break;
        case PROP_CTW_VISIBLE:
            ms->ctw_visible = g_value_get_boolean (value);
            break;
        case PROP_CTW_STYLE:
            ms->ctw_style = g_value_get_enum (value);
            break;
        case PROP_CTW_ORIENTATION:
            ms->ctw_orientation = g_value_get_enum (value);
            break;
        case PROP_CTW_GEOMETRY:
            g_free (ms->ctw_geometry);
            ms->ctw_geometry = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_settings_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    MtSettings *ms = MT_SETTINGS (object);

    switch (prop_id)
    {
        case PROP_DWELL_ENABLED:
            g_value_set_boolean (value, ms->dwell_enabled);
            break;
        case PROP_DWELL_THRESHOLD:
            g_value_set_int (value, ms->dwell_threshold);
            break;
        case PROP_DWELL_MODE:
            g_value_set_enum (value, ms->dwell_mode);
            break;
        case PROP_DWELL_GESTURE_SINGLE:
            g_value_set_enum (value, ms->dwell_gesture_single);
            break;
        case PROP_DWELL_GESTURE_DOUBLE:
            g_value_set_enum (value, ms->dwell_gesture_double);
            break;
        case PROP_DWELL_GESTURE_DRAG:
            g_value_set_enum (value, ms->dwell_gesture_drag);
            break;
        case PROP_DWELL_GESTURE_SECONDARY:
            g_value_set_enum (value, ms->dwell_gesture_secondary);
            break;
        case PROP_SSC_ENABLED:
            g_value_set_boolean (value, ms->ssc_enabled);
            break;
        case PROP_CTW_VISIBLE:
            g_value_set_boolean (value, ms->ctw_visible);
            break;
        case PROP_CTW_STYLE:
            g_value_set_enum (value, ms->ctw_style);
            break;
        case PROP_CTW_ORIENTATION:
            g_value_set_enum (value, ms->ctw_orientation);
            break;
        case PROP_CTW_GEOMETRY:
            g_value_set_string (value, ms->ctw_geometry);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_settings_class_init (MtSettingsClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = mt_settings_set_property;
    object_class->get_property = mt_settings_get_property;
    object_class->dispose = mt_settings_dispose;
    object_class->finalize = mt_settings_finalize;

    g_object_class_install_property (object_class,
                                     PROP_DWELL_ENABLED,
                                     g_param_spec_boolean ("dwell-enabled",
                                                           "Dwell enabled",
                                                           "Enable dwell clicks",
                                                           FALSE, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_THRESHOLD,
                                     g_param_spec_int ("dwell-threshold",
                                                       "Dwell threshold",
                                                       "Ignore small mouse movements below threshold",
                                                       0, 30, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_MODE,
                                     g_param_spec_enum ("dwell-mode",
                                                        "Dwell mode",
                                                        "Dwell click mode",
                                                        G_DESKTOP_TYPE_MOUSE_DWELL_MODE,
                                                        G_DESKTOP_MOUSE_DWELL_MODE_WINDOW,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_SINGLE,
                                     g_param_spec_enum ("dwell-gesture-single",
                                                        "Dwell gesture single",
                                                        "Gesture for single click",
                                                        G_DESKTOP_TYPE_MOUSE_DWELL_DIRECTION,
                                                        G_DESKTOP_MOUSE_DWELL_DIRECTION_LEFT,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_DOUBLE,
                                     g_param_spec_enum ("dwell-gesture-double",
                                                        "Dwell gesture double",
                                                        "Gesture for double click",
                                                        G_DESKTOP_TYPE_MOUSE_DWELL_DIRECTION,
                                                        G_DESKTOP_MOUSE_DWELL_DIRECTION_UP,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_DRAG,
                                     g_param_spec_enum ("dwell-gesture-drag",
                                                        "Dwell gesture drag",
                                                        "Gesture for drag action",
                                                        G_DESKTOP_TYPE_MOUSE_DWELL_DIRECTION,
                                                        G_DESKTOP_MOUSE_DWELL_DIRECTION_DOWN,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_SECONDARY,
                                     g_param_spec_enum ("dwell-gesture-secondary",
                                                        "Dwell gesture secondary",
                                                        "Gesture for secondary click",
                                                        G_DESKTOP_TYPE_MOUSE_DWELL_DIRECTION,
                                                        G_DESKTOP_MOUSE_DWELL_DIRECTION_RIGHT,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_SSC_ENABLED,
                                     g_param_spec_boolean ("ssc-enabled",
                                                           "SSC enabled",
                                                           "Enable simulated secondary clicks",
                                                           FALSE, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_CTW_VISIBLE,
                                     g_param_spec_boolean ("ctw-visible",
                                                           "Click-type window visibility",
                                                           "Click-type window visibility",
                                                           TRUE, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_CTW_STYLE,
                                     g_param_spec_enum ("ctw-style",
                                                        "Click-type window style",
                                                        "Button style of the click-type window",
                                                        MT_TYPE_CLICK_TYPE_WINDOW_STYLE,
                                                        MT_CLICK_TYPE_WINDOW_STYLE_BOTH,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_CTW_ORIENTATION,
                                     g_param_spec_enum ("ctw-orientation",
                                                        "Click-type window orientation",
                                                        "Orientation of the click-type window",
                                                        MT_TYPE_CLICK_TYPE_WINDOW_ORIENTATION,
                                                        MT_CLICK_TYPE_WINDOW_ORIENTATION_HORIZONTAL,
                                                        PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_CTW_GEOMETRY,
                                     g_param_spec_string ("ctw-geometry",
                                                          "Click-type window geometry",
                                                          "Size and position of the click-type window",
                                                          NULL, PFLAGS));
}

MtSettings *
mt_settings_get_default (void)
{
    static MtSettings *ms = NULL;

    if (!ms)
    {
        ms = g_object_new (MT_TYPE_SETTINGS, NULL);
        g_object_add_weak_pointer (G_OBJECT (ms), (gpointer *) &ms);
    }
    return ms;
}
