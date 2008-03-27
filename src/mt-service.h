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

#ifndef __MT_SERVICE_H__
#define __MT_SERVICE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_SERVICE            (mt_service_get_type ())
#define MT_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MT_TYPE_SERVICE, MtService))
#define MT_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MT_TYPE_SERVICE, MtServiceClass))
#define MT_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MT_TYPE_SERVICE))
#define MT_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MT_TYPE_SERVICE))
#define MT_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MT_TYPE_SERVICE, MtServiceClass))

typedef struct _MtService MtService;
typedef struct _MtServiceClass MtServiceClass;

struct _MtService {
    GObject parent;
};

struct _MtServiceClass {
    GObjectClass parent;
};

GType mt_service_get_type (void) G_GNUC_CONST;

MtService * mt_service_get_default   (void);
gboolean    mt_service_set_clicktype (MtService *service,
				      guint      clicktype,
				      GError   **error);
guint       mt_service_get_clicktype (MtService *service);

G_END_DECLS

#endif /* __MT_SERVICE_H__ */
