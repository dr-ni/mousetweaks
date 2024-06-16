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

#ifndef __MT_SERVICE_H__
#define __MT_SERVICE_H__

#include <glib-object.h>

#include "mt-common.h"

G_BEGIN_DECLS

#define MT_TYPE_SERVICE  (mt_service_get_type ())
#define MT_SERVICE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_SERVICE, MtService))
#define MT_IS_SERVICE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_SERVICE))

typedef GObjectClass             MtServiceClass;
typedef struct _MtService        MtService;
typedef struct _MtServicePrivate MtServicePrivate;

struct _MtService
{
    GObject           parent;
    MtServicePrivate *priv;
};

GType             mt_service_get_type          (void) G_GNUC_CONST;

MtService *       mt_service_get_default       (void);

MtDwellClickType  mt_service_get_click_type    (MtService       *service);
void              mt_service_set_click_type    (MtService       *service,
                                                MtDwellClickType type);

G_END_DECLS

#endif /* __MT_SERVICE_H__ */
