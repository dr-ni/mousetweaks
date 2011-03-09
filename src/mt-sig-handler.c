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

#include <unistd.h>
#include <signal.h>

#include "mt-sig-handler.h"

enum
{
    SIGNAL,
    LAST_SIGNAL
};

struct _MtSigHandlerPrivate
{
    GIOChannel *read_chan;
    guint       pipe_set_up : 1;
};

static guint signals[LAST_SIGNAL] = { 0, };

static int _write_fd;
static int _block_cnt;
static sigset_t _old_mask;

G_DEFINE_TYPE (MtSigHandler, mt_sig_handler, G_TYPE_OBJECT)

static void
mt_sig_handler_init (MtSigHandler *sigh)
{
    sigh->priv = G_TYPE_INSTANCE_GET_PRIVATE (sigh,
                                              MT_TYPE_SIG_HANDLER,
                                              MtSigHandlerPrivate);
    _write_fd = 0;
    _block_cnt = 0;
    sigemptyset (&_old_mask);
}

static void
mt_sig_handler_finalize (GObject *object)
{
    MtSigHandlerPrivate *priv = MT_SIG_HANDLER (object)->priv;

    if (priv->pipe_set_up)
    {
        g_io_channel_unref (priv->read_chan);
        close (_write_fd);
    }

    G_OBJECT_CLASS (mt_sig_handler_parent_class)->finalize (object);
}

static void
mt_sig_handler_class_init (MtSigHandlerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = mt_sig_handler_finalize;

    signals[SIGNAL] =
        g_signal_new (g_intern_static_string ("signal"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

    g_type_class_add_private (klass, sizeof (MtSigHandlerPrivate));
}

static void
signal_handler (int signal_id)
{
    unsigned char sid;
    int G_GNUC_UNUSED whtevr;

    sid = (unsigned char) signal_id;
    whtevr = write (_write_fd, &sid, 1);
}

static void
mt_sig_handler_block_signals (void)
{
    _block_cnt++;

    if (_block_cnt == 1)
    {
        sigset_t mask;

        sigfillset (&mask);
        sigprocmask (SIG_BLOCK, &mask, &_old_mask);
    }
}

static void
mt_sig_handler_unblock_signals (void)
{
    _block_cnt--;

    if (_block_cnt == 0)
        sigprocmask (SIG_SETMASK, &_old_mask, NULL);
}

static gboolean
mt_sig_handler_io_watch (GIOChannel   *read_chan,
                         GIOCondition  condition,
                         MtSigHandler *sigh)
{
    gchar buf[256];
    gsize bytes_read;

    mt_sig_handler_block_signals ();

    if (g_io_channel_read_chars (read_chan, buf, sizeof (buf),
                                 &bytes_read, NULL) == G_IO_STATUS_NORMAL)
    {
        gint i, sig_id;

        for (i = 0; i < bytes_read; i++)
        {
            sig_id = buf[i];
            g_signal_emit (sigh, signals[SIGNAL], 0, sig_id);
        }
    }

    mt_sig_handler_unblock_signals ();

    return TRUE;
}

MtSigHandler *
mt_sig_handler_get_default (void)
{
    static MtSigHandler *sigh = NULL;

    if (!sigh)
    {
        sigh = g_object_new (MT_TYPE_SIG_HANDLER, NULL);
        g_object_add_weak_pointer (G_OBJECT (sigh), (gpointer *) &sigh);
    }
    return sigh;
}

gboolean
mt_sig_handler_setup_pipe (MtSigHandler *sigh)
{
    MtSigHandlerPrivate *priv;
    int sig_pipe[2];

    g_return_val_if_fail (MT_IS_SIG_HANDLER (sigh), FALSE);

    priv = sigh->priv;

    if (priv->pipe_set_up)
    {
        return TRUE;
    }

    if (pipe (sig_pipe) != 0)
    {
        return FALSE;
    }

    priv->pipe_set_up = TRUE;

    _write_fd = sig_pipe[1];

    priv->read_chan = g_io_channel_unix_new (sig_pipe[0]);
    g_io_channel_set_flags (priv->read_chan, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_encoding (priv->read_chan, NULL, NULL);
    g_io_channel_set_close_on_unref (priv->read_chan, TRUE);
    g_io_add_watch (priv->read_chan, G_IO_IN,
                    (GIOFunc) mt_sig_handler_io_watch, sigh);

    g_io_channel_unref (priv->read_chan);

    return TRUE;
}

void
mt_sig_handler_catch (MtSigHandler *sigh, int signal_id)
{
    struct sigaction action, old;

    g_return_if_fail (MT_IS_SIG_HANDLER (sigh));

    sigemptyset (&action.sa_mask);
    action.sa_handler = signal_handler;
    action.sa_flags = 0;

    sigaction (signal_id, &action, &old);
}
