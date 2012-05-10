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

/*
 * Modified version of dpid.c - libdaemon.
 * Copyright 2003-2007 Lennart Poettering <mzqnrzba (at) 0pointer (dot) de>
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stddef.h>

#include <glib.h>

#include "mt-pidfile.h"

#ifndef ETIME
#define ETIME ETIMEDOUT /* For FreeBSD */
#endif

static const char *
mt_pidfile_proc (void)
{
    static char fn[512];
    static int  fn_set = 0;

    if (!fn_set)
    {
        snprintf (fn, sizeof (fn),
                  "%s%smousetweaks.pid",
                  g_get_user_runtime_dir (),
                  G_DIR_SEPARATOR_S);
        fn_set = 1;
    }

    return fn;
}

static int
lock_file (int fd, int enable)
{
    struct flock f;

    memset(&f, 0, sizeof(f));
    f.l_type = enable ? F_WRLCK : F_UNLCK;
    f.l_whence = SEEK_SET;
    f.l_start = 0;
    f.l_len = 0;

    if (fcntl(fd, F_SETLKW, &f) < 0)
        return -1;

    return 0;
}

pid_t
mt_pidfile_is_running (void)
{
    const char *fn;
    static char txt[256];
    int fd = -1, locked = -1;
    pid_t ret = (pid_t) -1, pid;
    ssize_t l;
    long lpid;
    char *e = NULL;

    if (!(fn = mt_pidfile_proc ()))
    {
        errno = EINVAL;
        goto FINISH;
    }

    if ((fd = open (fn, O_RDWR, 0644)) < 0)
        goto FINISH;

    if ((locked = lock_file (fd, 1)) < 0)
        goto FINISH;

    if ((l = read (fd, txt, sizeof(txt) - 1)) < 0)
    {
        unlink(fn);
        goto FINISH;
    }

    txt[l] = 0;
    txt[strcspn(txt, "\r\n")] = 0;

    errno = 0;
    lpid = strtol(txt, &e, 10);
    pid = (pid_t) lpid;

    if (errno != 0 || !e || *e || (long) pid != lpid)
    {
        unlink (fn);
        errno = EINVAL;
        goto FINISH;
    }

    if (kill (pid, 0) != 0 && errno != EPERM)
    {
        int saved_errno = errno;
        unlink (fn);
        errno = saved_errno;
        goto FINISH;
    }

    ret = pid;

FINISH:
    if (fd >= 0)
    {
        int saved_errno = errno;

        if (locked >= 0)
            lock_file (fd, 0);

        errno = saved_errno;
        close (fd);
    }

    return ret;
}

int
mt_pidfile_kill_wait (int signal, int sec)
{
    pid_t pid;
    time_t t;

    if ((pid = mt_pidfile_is_running ()) < 0)
        return -1;

    if (kill (pid, signal) < 0)
        return -1;

    t = time (NULL) + sec;
    for (;;)
    {
        int r;
        struct timeval tv = { 0, 100000 };

        if (time (NULL) > t)
        {
            errno = ETIME;
            return -1;
        }

        if ((r = kill (pid, 0)) < 0 && errno != ESRCH)
            return -1;

        if (r)
            return 0;

        if (select (0, NULL, NULL, NULL, &tv) < 0)
            return -1;
    }
}

int
mt_pidfile_create (void)
{
    const char *fn;
    int fd = -1;
    int ret = -1;
    int locked = -1;
    char t[64];
    ssize_t l;
    mode_t u;

    u = umask(022);

    if (!(fn = mt_pidfile_proc ()))
    {
        errno = EINVAL;
        goto FINISH;
    }

    if ((fd = open (fn, O_CREAT|O_RDWR|O_EXCL, 0644)) < 0)
        goto FINISH;

    if ((locked = lock_file (fd, 1)) < 0)
    {
        int saved_errno = errno;
        unlink (fn);
        errno = saved_errno;
        goto FINISH;
    }

    snprintf (t, sizeof(t), "%lu\n", (unsigned long) getpid ());
    l = strlen (t);

    if (write (fd, t, l) != l)
    {
        int saved_errno = errno;
        unlink (fn);
        errno = saved_errno;
        goto FINISH;
    }

    ret = 0;

FINISH:
    if (fd >= 0)
    {
        int saved_errno = errno;

        if (locked >= 0)
            lock_file (fd, 0);

        close (fd);
        errno = saved_errno;
    }

    umask(u);

    return ret;
}

int
mt_pidfile_remove (void)
{
    const char *fn;

    if (!(fn = mt_pidfile_proc ()))
    {
        errno = EINVAL;
        return -1;
    }

    if (unlink (fn) < 0)
        return -1;

    return 0;
}
