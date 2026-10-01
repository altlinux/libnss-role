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

#include "role/glob.h"
#include "role/parser.h"
#include "role/fileop.h"
#include "role/lock_file.h"
#include "role/paths.h"

/*
 * \brief Selector function for scandir which gets all regular files.
 *
 * \param[in] entry Entry to test.
 * \return
 *  - 0: Entry is not a regular file
 *  - 1: Entry is a regular file
 */
/*
 * \brief Check that file name is a role name with role file extension.
 *
 * \param[in] filename File name to check.
 * \return
 *  - 0: File name is too short or has another extension
 *  - 1: File name has role file extension
 */
static int librole_has_role_extension(const char *filename)
{
    const char *extension_pattern = LIBROLE_ROLE_EXTENSION;
    size_t extensionlen = strlen(extension_pattern);
    size_t namelen = strlen(filename);

    /* Need at least one symbol of the role name before the extension */
    if (namelen <= extensionlen)
        return 0;

    return !strcmp(extension_pattern, filename + namelen - extensionlen);
}

static int librole_is_role_file(const struct dirent *entry)
{
    if (!entry)
        return 0;

    return DT_REG == entry->d_type && librole_has_role_extension(entry->d_name);
}

int librole_validate_filename_from_dir(const char *filename)
{
    if (!filename)
    {
        return LIBROLE_INTERNAL_ERROR;
    }

    if (librole_has_role_extension(filename))
    {
        return LIBROLE_OK;
    }

    return LIBROLE_INVALID_ROLE_FILENAME;
}

/*
 * \brief Validate single system role file name from roles directory.
 *
 * \param[in] filename Validate File name in a roles directory.
 * \return
 *  - LIBROLE_OK: if filename is valid.
 *  - LIBROLE_INVALID_ROLE_FILENAME: if filename is invalid.
 */
int librole_validate_system_role_filename(const char *filename, char **rolename)
{
    int retcode = LIBROLE_INVALID_ROLE_FILENAME;
    size_t extensionlen = strlen(LIBROLE_ROLE_EXTENSION);
    char *system_role = NULL;
    struct group *gr = NULL;
    gid_t gid;
    struct librole_graph G;

    retcode = librole_validate_filename_from_dir(filename);
    if (retcode != LIBROLE_OK)
        return retcode;

    system_role = strdup(filename);
    if (system_role == NULL)
        return LIBROLE_MEMORY_ERROR;
    system_role[strlen(filename) - extensionlen] = 0;

    gr = getgrnam(system_role);
    if (!gr) {
        retcode = LIBROLE_NO_SUCH_GROUP;
        goto librole_validate_system_role_filename_from_dir_done;
    }
    gid = gr->gr_gid;

    retcode = librole_graph_init(&G);
    if (retcode != LIBROLE_OK) {
        librole_graph_free(&G);
        goto librole_validate_system_role_filename_from_dir_done;
    }

    retcode = librole_read_file_from_dir(librole_config_dir(), filename, &G);
    if (retcode == LIBROLE_OK)
        retcode = librole_find_gid(&G, gid, NULL);
    librole_graph_free(&G);

librole_validate_system_role_filename_from_dir_done:
    if (retcode == LIBROLE_OK && rolename != NULL) {
        *rolename = system_role;
    } else {
        free(system_role);
    }

    return retcode;
}

int librole_read_file_from_dir(const char * const directory,
    const char * const filename,
    struct librole_graph *role_graph)
{
    int retcode = LIBROLE_OK;
    size_t fullpathlen;
    char *fullpath = NULL;

    errno = 0;

    if (!directory || !filename || !role_graph)
    {
        retcode = LIBROLE_INCORRECT_VALUE;
        goto librole_read_file_from_dir_done;
    }

    fullpathlen = strlen(directory) + strlen(filename) + 1 + 1;
    if (fullpathlen > PATH_MAX)
    {
        retcode = ENAMETOOLONG;
        goto librole_read_file_from_dir_done;
    }
    fullpath = calloc(fullpathlen, sizeof(char));
    if (!fullpath)
    {
        retcode = LIBROLE_MEMORY_ERROR;
        goto librole_read_file_from_dir_done;
    }

    /* Build full path to the file being read for roles */
    strcpy(fullpath, directory);
    strcat(fullpath, "/");
    strcat(fullpath, filename);

    /* Ignore return code from librole_reading to retain previous
     * (without /etc/role.d) behavior. */
    retcode = librole_reading(fullpath, role_graph);

librole_read_file_from_dir_done:
    free(fullpath);
    fullpath = NULL;

    return retcode;
}

int librole_get_directory_files(const char * const directory,
    struct librole_graph *role_graph)
{
    int retcode = LIBROLE_OK;
    int result;
    struct dirent **files;
    int file_count = 0;
    int i = 0;

    errno = 0;

    if (!directory || !role_graph)
    {
        retcode = LIBROLE_INCORRECT_VALUE;
        goto librole_get_directory_files_end;
    }


    /* Get all regular files in directory */
    file_count = scandir(directory, &files, librole_is_role_file, alphasort);
    if (file_count < 0)
    {
        retcode = errno == ENOENT ? LIBROLE_OK : LIBROLE_IO_ERROR;
        goto librole_get_directory_files_end;
    }

    for (i = 0; i < file_count; ++i)
    {
        /* Validate reading filename and skip if name is not valid */
        if (librole_validate_filename_from_dir(files[i]->d_name) == LIBROLE_OK) {
            /* Don't do anything on errors and try to continue reading */
            result = librole_read_file_from_dir(
                directory, files[i]->d_name, role_graph);
            /* Later successful files must not hide an incomplete graph.
             * Prefer a hard error over an unavailable group source. */
            if (result != LIBROLE_OK &&
                (retcode == LIBROLE_OK || retcode == LIBROLE_SOURCE_UNAVAIL))
                retcode = result;
        }
        free(files[i]);
    }
    free(files);

librole_get_directory_files_end:
    return retcode;
}

int librole_get_system_roles(const char * const directory,
    char *system_roles[])
{
    int retcode = LIBROLE_OK;
    struct dirent **files;
    int file_count = 0;
    int i, r;

    errno = 0;

    if (!directory || !system_roles)
    {
        retcode = LIBROLE_INCORRECT_VALUE;
        goto librole_get_directory_files_end;
    }


    /* Get all regular files in directory */
    file_count = scandir(directory, &files, librole_is_role_file, alphasort);
    if (file_count < 0)
    {
        retcode = errno;
        goto librole_get_directory_files_end;
    }

    for (i = 0, r = 0; i < file_count; ++i)
    {
        char* rolename;
        /* Validate reading filename and skip if name is not valid */
        if (librole_validate_system_role_filename(files[i]->d_name, &rolename) == LIBROLE_OK) {
            /* Keep the last element for the NULL terminator */
            if (r < LIBROLE_MAX_SYSTEM_ROLES - 1) {
                system_roles[r++] = rolename;
            } else {
                free(rolename);
            }
        }
        free(files[i]);
    }
    free(files);
    system_roles[r] = NULL;

librole_get_directory_files_end:
    return retcode;
}

int librole_find_role_file(const char * const directory, gid_t gid, char **filename)
{
    int retcode = LIBROLE_NO_SUCH_GROUP;
    struct dirent **files;
    int file_count;
    int i;

    if (!directory || !filename)
        return LIBROLE_INCORRECT_VALUE;

    *filename = NULL;
    file_count = scandir(directory, &files, librole_is_role_file, alphasort);
    if (file_count < 0)
        return LIBROLE_NO_SUCH_GROUP;

    for (i = 0; i < file_count; ++i)
    {
        struct librole_graph G;

        if (retcode == LIBROLE_NO_SUCH_GROUP) {
            retcode = librole_graph_init(&G);
            if (retcode == LIBROLE_OK) {
                /* Unresolvable entries don't matter: we look for gid */
                librole_read_file_from_dir(directory, files[i]->d_name, &G);
                retcode = librole_find_gid(&G, gid, NULL);
                if (retcode == LIBROLE_OK) {
                    *filename = strdup(files[i]->d_name);
                    if (!*filename)
                        retcode = LIBROLE_MEMORY_ERROR;
                }
            }
            librole_graph_free(&G);
        }
        free(files[i]);
    }
    free(files);

    return retcode;
}
