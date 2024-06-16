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

#ifndef __MT_SIG_HANDLER_H__
#define __MT_SIG_HANDLER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_SIG_HANDLER  (mt_sig_handler_get_type ())
#define MT_SIG_HANDLER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_SIG_HANDLER, MtSigHandler))
#define MT_IS_SIG_HANDLER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_SIG_HANDLER))

typedef GObjectClass                MtSigHandlerClass;
typedef struct _MtSigHandler        MtSigHandler;
typedef struct _MtSigHandlerPrivate MtSigHandlerPrivate;

struct _MtSigHandler
{
    GObject              parent;
    MtSigHandlerPrivate *priv;
};

GType              mt_sig_handler_get_type        (void) G_GNUC_CONST;

MtSigHandler *     mt_sig_handler_get_default     (void);
gboolean           mt_sig_handler_setup_pipe      (MtSigHandler *sigh);

void               mt_sig_handler_catch           (MtSigHandler *sigh,
                                                   int           signal_id);

G_END_DECLS

#endif /* __MT_SIG_HANDLER_H__ */
