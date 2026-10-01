/*
 * Copyright (c) 2008-2020 Etersoft
 * Copyright (c) 2020 BaseALT
 *
 * NSS library for roles and privileges.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.1
 * of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "role/glob.h"

static int get_pid(char *buf, pid_t *pid)
{
    if (sscanf(buf, "%i", pid) < 1)
        return LIBROLE_IO_ERROR;

    return LIBROLE_OK;
}

static int check_link_count(const char *file)
{
    struct stat sb;

    if (stat (file, &sb) != 0) {
        return LIBROLE_UNKNOWN_ERROR;
    }

    if (sb.st_nlink != 2) {
        return LIBROLE_UNKNOWN_ERROR;
    }

    return LIBROLE_OK;
}


/*
 * Lock is a hard link file.lock to a temporary file tmp (file.PID)
 * containing PID of the owner, like in shadow-utils. The file itself
 * is not touched.
 */
static int do_lock(const char *tmp, const char *lock)
{
    int fd;
    pid_t pid;
    ssize_t len;
    int result;
    char buf[32];

    /* Remove our own leftover */
    unlink(tmp);
    fd = open(tmp, O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (-1 == fd) {
        return LIBROLE_IO_ERROR;
    }

    pid = getpid();
    snprintf(buf, sizeof buf, "%lu", (unsigned long) pid);
    len = (ssize_t) strlen (buf) + 1;
    if (write(fd, buf, len) != len) {
        close (fd);
        unlink(tmp);
        return LIBROLE_IO_ERROR;
    }
    close (fd);

    if (link(tmp, lock) == 0) {
        result = check_link_count (tmp);
        unlink(tmp);
        return result;
    }

    fd = open(lock, O_RDONLY);
    if (fd == -1) {
        unlink(tmp);
        errno = EINVAL;
        return LIBROLE_IO_ERROR;
    }
    len = read(fd, buf, sizeof (buf) - 1);
    close(fd);
    if (len <= 0) {
        unlink(tmp);
        errno = EINVAL;
        return LIBROLE_IO_ERROR;
    }
    buf[len] = '\0';
    if ((result = get_pid(buf, &pid)) != LIBROLE_OK) {
        unlink(tmp);
        errno = EINVAL;
        return result;
    }
    /* The owner is alive (EPERM: it runs under another user) */
    if (kill(pid, 0) == 0 || errno == EPERM) {
        unlink(tmp);
        errno = EEXIST;
        return LIBROLE_LOCKED;
    }
    /* Stale lock */
    if (unlink(lock) != 0) {
        unlink(tmp);
        return LIBROLE_IO_ERROR;
    }

    result = LIBROLE_IO_ERROR;
    if ((link(tmp, lock) == 0) && (check_link_count(tmp) == LIBROLE_OK))
        result = LIBROLE_OK;

    unlink(tmp);
    return result;
}

int librole_lock(const char *file)
{
    char lock[1024];
    char tmp[1024];
    int len;

    len = snprintf(lock, sizeof lock, "%s.lock", file);
    if (len < 1 || (size_t) len >= sizeof lock)
        return LIBROLE_UNKNOWN_ERROR;

    len = snprintf(tmp, sizeof tmp, "%s.%lu", file, (unsigned long) getpid());
    if (len < 1 || (size_t) len >= sizeof tmp)
        return LIBROLE_UNKNOWN_ERROR;

    return do_lock(tmp, lock);
}

int librole_unlock(const char *file)
{
    char lock[1024];

    if (snprintf(lock, sizeof lock, "%s.lock", file) < 1)
        return LIBROLE_UNKNOWN_ERROR;

    unlink(lock);
    return LIBROLE_OK;
}
