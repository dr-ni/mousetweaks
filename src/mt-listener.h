/*
 * Copyright © 2007-2008 Gerd Kohlberger <lowfi@chello.at>
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
#include <cspi/spi.h>

G_BEGIN_DECLS

#define MT_TYPE_EVENT            (mt_event_get_type ())
#define MT_TYPE_LISTENER         (mt_listener_get_type ())
#define MT_LISTENER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_LISTENER, MtListener))
#define MT_LISTENER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MT_TYPE_LISTENER, MtListenerClass))
#define MT_IS_LISTENER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_LISTENER))
#define MT_IS_LISTENER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MT_TYPE_LISTENER))
#define MT_LISTENER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MT_TYPE_LISTENER, MtListenerClass))

typedef struct _MtListener MtListener;
typedef struct _MtListenerClass MtListenerClass;

struct _MtListener {
    GObject parent;
};

struct _MtListenerClass {
    GObjectClass parent;
};

GType        mt_listener_get_type       (void) G_GNUC_CONST;
MtListener * mt_listener_get_default    (void);
Accessible * mt_listener_current_focus  (MtListener *listener);

enum {
    EV_MOTION = 0,
    EV_BUTTON_PRESS,
    EV_BUTTON_RELEASE
};

typedef struct _MtEvent MtEvent;
struct _MtEvent {
    gint type;
    gint x;
    gint y;
};

GType     mt_event_get_type (void) G_GNUC_CONST;
MtEvent * mt_event_copy     (const MtEvent *event);
void      mt_event_free     (MtEvent       *event);

G_END_DECLS

#endif /* __MT_LISTENER_H__ */
