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
#include "mt-common.h"

#define PFLAGS (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

#define BIND_PROP(p,k) (g_settings_bind (ms->settings, (k), ms, (p), \
                        G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY))

enum
{
    PROP_0,
    PROP_DWELL_ENABLED,
    PROP_DWELL_TIME,
    PROP_DWELL_THRESHOLD,
    PROP_DWELL_MODE,
    PROP_DWELL_GESTURE_SINGLE,
    PROP_DWELL_GESTURE_DOUBLE,
    PROP_DWELL_GESTURE_DRAG,
    PROP_DWELL_GESTURE_SECONDARY,
    PROP_SSC_ENABLED,
    PROP_SSC_TIME,
    PROP_CTW_VISIBLE,
    PROP_CTW_STYLE,
    PROP_ANIMATE_CURSOR,
};

G_DEFINE_TYPE (MtSettings, mt_settings, G_TYPE_OBJECT)

static void
mt_settings_init (MtSettings *ms)
{
    ms->settings = g_settings_new (MT_SCHEMA_ID);

    BIND_PROP ("dwell-enabled", KEY_DWELL_ENABLED);
    BIND_PROP ("dwell-time", KEY_DWELL_TIME);
    BIND_PROP ("dwell-threshold", KEY_DWELL_THRESHOLD);
    BIND_PROP ("dwell-mode", KEY_DWELL_MODE);
    BIND_PROP ("dwell-gesture-single", KEY_DWELL_GESTURE_SINGLE);
    BIND_PROP ("dwell-gesture-double", KEY_DWELL_GESTURE_DOUBLE);
    BIND_PROP ("dwell-gesture-drag", KEY_DWELL_GESTURE_DRAG);
    BIND_PROP ("dwell-gesture-secondary", KEY_DWELL_GESTURE_SECONDARY);
    BIND_PROP ("ssc-enabled", KEY_SSC_ENABLED);
    BIND_PROP ("ssc-time", KEY_SSC_TIME);
    BIND_PROP ("ctw-visible", KEY_CTW_VISIBLE);
    BIND_PROP ("ctw-style", KEY_CTW_STYLE);
    BIND_PROP ("animate-cursor", KEY_ANIMATE_CURSOR);
}

static void
mt_settings_dispose (GObject *object)
{
    MtSettings *ms = MT_SETTINGS (object);

    if (ms->settings)
    {
        g_object_unref (ms->settings);
        ms->settings = NULL;
    }

    G_OBJECT_CLASS (mt_settings_parent_class)->dispose (object);
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
        case PROP_DWELL_TIME:
            ms->dwell_time = g_value_get_double (value);
            break;
        case PROP_DWELL_THRESHOLD:
            ms->dwell_threshold = g_value_get_int (value);
            break;
        case PROP_DWELL_MODE:
            ms->dwell_mode = g_value_get_int (value);
            break;
        case PROP_DWELL_GESTURE_SINGLE:
            ms->dwell_gesture_single = g_value_get_int (value);
            break;
        case PROP_DWELL_GESTURE_DOUBLE:
            ms->dwell_gesture_double = g_value_get_int (value);
            break;
        case PROP_DWELL_GESTURE_DRAG:
            ms->dwell_gesture_drag = g_value_get_int (value);
            break;
        case PROP_DWELL_GESTURE_SECONDARY:
            ms->dwell_gesture_secondary = g_value_get_int (value);
            break;
        case PROP_SSC_ENABLED:
            ms->ssc_enabled = g_value_get_boolean (value);
            break;
        case PROP_SSC_TIME:
            ms->ssc_time = g_value_get_double (value);
            break;
        case PROP_CTW_VISIBLE:
            ms->ctw_visible = g_value_get_boolean (value);
            break;
        case PROP_CTW_STYLE:
            ms->ctw_style = g_value_get_int (value);
            break;
        case PROP_ANIMATE_CURSOR:
            ms->animate_cursor = g_value_get_boolean (value);
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
        case PROP_DWELL_TIME:
            g_value_set_double (value, ms->dwell_time);
            break;
        case PROP_DWELL_THRESHOLD:
            g_value_set_int (value, ms->dwell_threshold);
            break;
        case PROP_DWELL_MODE:
            g_value_set_int (value, ms->dwell_mode);
            break;
        case PROP_DWELL_GESTURE_SINGLE:
            g_value_set_int (value, ms->dwell_gesture_single);
            break;
        case PROP_DWELL_GESTURE_DOUBLE:
            g_value_set_int (value, ms->dwell_gesture_double);
            break;
        case PROP_DWELL_GESTURE_DRAG:
            g_value_set_int (value, ms->dwell_gesture_drag);
            break;
        case PROP_DWELL_GESTURE_SECONDARY:
            g_value_set_int (value, ms->dwell_gesture_secondary);
            break;
        case PROP_SSC_ENABLED:
            g_value_set_boolean (value, ms->ssc_enabled);
            break;
        case PROP_SSC_TIME:
            g_value_set_double (value, ms->ssc_time);
            break;
        case PROP_CTW_VISIBLE:
            g_value_set_boolean (value, ms->ctw_visible);
            break;
        case PROP_CTW_STYLE:
            g_value_set_int (value, ms->ctw_style);
            break;
        case PROP_ANIMATE_CURSOR:
            g_value_set_boolean (value, ms->animate_cursor);
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

    g_object_class_install_property (object_class,
                                     PROP_DWELL_ENABLED,
                                     g_param_spec_boolean ("dwell-enabled",
                                                           "Dwell enabled",
                                                           "Enable dwell clicks",
                                                           FALSE, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_TIME,
                                     g_param_spec_double ("dwell-time",
                                                          "Dwell time",
                                                          "Dwell click time",
                                                          0.1, 3.0, 1.2, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_THRESHOLD,
                                     g_param_spec_int ("dwell-threshold",
                                                       "Dwell threshold",
                                                       "Ignore small mouse movements below threshold",
                                                       0, 30, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_MODE,
                                     g_param_spec_int ("dwell-mode",
                                                       "Dwell mode",
                                                       "Dwell click mode",
                                                       0, 1, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_SINGLE,
                                     g_param_spec_int ("dwell-gesture-single",
                                                       "Dwell gesture single",
                                                       "Gesture for single click",
                                                       0, 3, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_DOUBLE,
                                     g_param_spec_int ("dwell-gesture-double",
                                                       "Dwell gesture double",
                                                       "Gesture for double click",
                                                       0, 3, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_DRAG,
                                     g_param_spec_int ("dwell-gesture-drag",
                                                       "Dwell gesture drag",
                                                       "Gesture for drag action",
                                                       0, 3, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_DWELL_GESTURE_SECONDARY,
                                     g_param_spec_int ("dwell-gesture-secondary",
                                                       "Dwell gesture secondary",
                                                       "Gesture for secondary click",
                                                       0, 3, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_SSC_ENABLED,
                                     g_param_spec_boolean ("ssc-enabled",
                                                           "SSC enabled",
                                                           "Enable simulated secondary clicks",
                                                           FALSE, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_SSC_TIME,
                                     g_param_spec_double ("ssc-time", "SSC time",
                                                          "Simulated secondary click time",
                                                          0.1, 3.0, 1.2, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_CTW_VISIBLE,
                                     g_param_spec_boolean ("ctw-visible",
                                                           "CTW visible",
                                                           "Show click-type window",
                                                           FALSE, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_CTW_STYLE,
                                     g_param_spec_int ("ctw-style",
                                                       "CTW style",
                                                       "Button style in click-type window",
                                                       0, 2, 0, PFLAGS));
    g_object_class_install_property (object_class,
                                     PROP_ANIMATE_CURSOR,
                                     g_param_spec_boolean ("animate-cursor",
                                                           "Animate cursor",
                                                           "Draw cursor animation",
                                                           TRUE, PFLAGS));
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
