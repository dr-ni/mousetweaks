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

#ifndef __MT_LISTENER_H__
#define __MT_LISTENER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_EVENT     (mt_event_get_type ())
#define MT_TYPE_LISTENER  (mt_listener_get_type ())
#define MT_LISTENER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_LISTENER, MtListener))
#define MT_IS_LISTENER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_LISTENER))

typedef GObjectClass              MtListenerClass;
typedef struct _MtListener        MtListener;
typedef struct _MtListenerPrivate MtListenerPrivate;

struct _MtListener
{
    GObject            parent;
    MtListenerPrivate *priv;
};

typedef enum
{
    MT_EVENT_MOTION = 0,
    MT_EVENT_BUTTON_PRESS,
    MT_EVENT_BUTTON_RELEASE
} MtEventType;

typedef struct _MtEvent MtEvent;
struct _MtEvent
{
    MtEventType type;
    gint        x;
    gint        y;
    gint        button;
};

GType              mt_event_get_type              (void) G_GNUC_CONST;
GType              mt_listener_get_type           (void) G_GNUC_CONST;

MtListener *       mt_listener_get_default        (void);
void               mt_listener_grab_mouse_wheel   (MtListener *listener);
void               mt_listener_ungrab_mouse_wheel (MtListener *listener);

G_END_DECLS

#endif /* __MT_LISTENER_H__ */
