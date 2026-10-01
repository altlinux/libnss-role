/*
 * Copyright (c) 2021 BaseALT
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
#include <stddef.h>
#include <stdlib.h>
/* For dirent and scandir */
#include <dirent.h>
#include <stdio.h>
/* For errno */
#include <errno.h>
/* For pathconf */
#include <unistd.h>
/* For strlen */
#include <string.h>
/* For PATH_MAX */
#include <linux/limits.h>
/* For stat, chmod */
#include <sys/stat.h>
/* For open */
#include <fcntl.h>

#include "role/glob.h"
#include "role/fileop_rw.h"
#include "role/pam_check.h"
#include "role/lock_file.h"
#include "role/paths.h"

/* delim: 0 - '', > 0 - ',' before, < 0 - ':' after */
static int write_group(FILE *f, int gid, int delim, int numeric_flag)
{
    /* print ',' before if needed */
    if (delim > 0 && fputc(',', f) < 0)
        return LIBROLE_IO_ERROR;

    if (numeric_flag) {
        if (fprintf(f, "%u", gid) < 0)
            return LIBROLE_IO_ERROR;
    } else {
        int result;
        char gr_name[LIBROLE_MAX_NAME];
        result = librole_get_group_name(gid, gr_name, LIBROLE_MAX_NAME);
        if (result != LIBROLE_OK)
            return result;
        if (fprintf(f, "%s", gr_name) < 0)
            return LIBROLE_IO_ERROR;
    }

    /* print ':' after if needed */
    if (delim < 0 && fputc(':', f) < 0)
        return LIBROLE_IO_ERROR;

    return LIBROLE_OK;
}


/* TODO: the same like in rolelst
 TODO: write in a new file and atomically rename */
int librole_writing(const char *file, struct librole_graph *G, int numeric_flag, int empty_flag, librole_roles_filter filter)
{
    int i, j, result;
    FILE *f = fopen(file, "w");
    if (!f)
        return LIBROLE_IO_ERROR;

    for(i = 0; i < G->size; i++) {
        if (filter) {
            char gr_name[LIBROLE_MAX_NAME];
            if (librole_get_group_name(G->gr[i].gid, gr_name, LIBROLE_MAX_NAME) != LIBROLE_OK)
                continue;
            if (!filter(gr_name))
                continue;
        } else if (!G->gr[i].size && !empty_flag)
            continue;

        result = write_group(f, G->gr[i].gid, -1, numeric_flag);
        if (result != LIBROLE_OK)
            goto libnss_role_writing_exit;

        for(j = 0; j < G->gr[i].size; j++) {
            result = write_group(f, G->gr[i].list[j], j, numeric_flag);
            if (result != LIBROLE_OK)
                goto libnss_role_writing_exit;
        }
        if (fputc('\n', f) < 0) {
            result = LIBROLE_IO_ERROR;
            goto libnss_role_writing_exit;
        }
    }

    result = LIBROLE_OK;

libnss_role_writing_exit:
    /* Buffered data is written here: report ENOSPC and so on */
    if (fclose(f) != 0 && result == LIBROLE_OK)
        result = LIBROLE_IO_ERROR;
    return result;
}

static int sync_file(const char *file)
{
    int result = LIBROLE_OK;
    int fd = open(file, O_RDONLY);

    if (fd == -1)
        return LIBROLE_IO_ERROR;
    if (fsync(fd) != 0)
        result = LIBROLE_IO_ERROR;
    close(fd);
    return result;
}

/*
 * Write G to file under lock. The file is replaced atomically: on any
 * error it is left untouched.
 */
int librole_write_file(const char *file, struct librole_graph *G, int empty_flag)
{
    int result;
    char tmp[PATH_MAX];
    struct stat sb;
    int len;

    len = snprintf(tmp, sizeof tmp, "%s.new", file);
    if (len < 1 || (size_t) len >= sizeof tmp)
        return LIBROLE_ERROR_PATH_TOO_LONG;

    result = librole_lock(file);
    if (result != LIBROLE_OK)
        return result;

    /* Leftover of an interrupted write, we hold the lock */
    unlink(tmp);
    result = librole_writing(tmp, G, 0, empty_flag, NULL);
    if (result != LIBROLE_OK)
        goto librole_write_file_fail;

    /* Keep permissions and owner of the file being replaced */
    if (stat(file, &sb) == 0) {
        if (chmod(tmp, sb.st_mode & 07777) != 0 ||
            (chown(tmp, sb.st_uid, sb.st_gid) != 0 && errno != EPERM)) {
            result = LIBROLE_IO_ERROR;
            goto librole_write_file_fail;
        }
    }

    result = sync_file(tmp);
    if (result != LIBROLE_OK)
        goto librole_write_file_fail;

    if (rename(tmp, file) != 0) {
        result = LIBROLE_IO_ERROR;
        goto librole_write_file_fail;
    }

    librole_unlock(file);
    return LIBROLE_OK;

librole_write_file_fail:
    unlink(tmp);
    librole_unlock(file);
    return result;
}

int librole_write(const char* pam_role, struct librole_graph *G, int empty_flag)
{
    int result;
    int pam_status = PAM_SUCCESS;
    pam_handle_t *pamh = NULL;

    result = librole_pam_check(pamh, pam_role, &pam_status);
    if (result != LIBROLE_OK) {
        goto exit;
    }

    result = librole_write_file(librole_config_file(), G, empty_flag);

/* TODO: can we release immediately? */
exit:
    librole_pam_release(pamh, pam_status);
    return result;
}

int librole_write_dir(const char* filename, const char* pam_role, struct librole_graph *G, int empty_flag)
{
    int result = 0;
    int pam_status = PAM_SUCCESS;
    size_t dirlen = strlen(librole_config_dir());
    size_t namelen = strlen(filename);
    size_t fullpathlen = dirlen + namelen + 1 + 1;
    pam_handle_t *pamh = NULL;
    char *fullpath = NULL;

    result = librole_pam_check(pamh, pam_role, &pam_status);
    if (result != LIBROLE_OK)
        return result;


    if (fullpathlen > PATH_MAX)
    {
        result = ENAMETOOLONG;
        goto librole_write_dir_done;
    }
    fullpath = calloc(fullpathlen, sizeof(char));
    if (!fullpath)
    {
        result = LIBROLE_MEMORY_ERROR;
        goto librole_write_dir_done;
    }

    /* Build full path to the file being read for roles */
    strcpy(fullpath, librole_config_dir());
    strcat(fullpath, "/");
    strcat(fullpath, filename);

    result = librole_write_file(fullpath, G, empty_flag);

/* TODO: can we release immediately? */
librole_write_dir_done:
    librole_pam_release(pamh, pam_status);
    free(fullpath);
    fullpath = NULL;

    return result;
}
