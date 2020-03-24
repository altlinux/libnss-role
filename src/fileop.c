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

#include "role/glob.h"
#include "role/parser.h"

#define MAX_PATH_LEN 4096

/*
 * \brief Selector function for scandir which gets all regular files.
 *
 * \param[in] entry Entry to test.
 * \return
 *  - 0: Entry is not a regular file
 *  - 1: Entry is a regular file
 */
static int librole_is_file(const struct dirent *entry)
{
    if (NULL == entry)
    {
        goto librole_is_file_end;
    }

    if (DT_REG == entry->d_type)
    {
        return 1;
    }

librole_is_file_end:
    return 0;
}

/*
 * \brief Read single role file from specified directory right into
 * role graph node.
 *
 * \param[in] directory Directory to read file from.
 * \param[in] filename File name in a directory to read roles from.
 * \param[in,out] role_graph Role graph to expand with roles.
 * \return
 *  - LIBROLE_OK: Don't try to fail.
 */
static int librole_read_file_from_dir(const char const *directory,
    const char const *filename,
    struct librole_graph *role_graph)
{
    errno = 0;
    int retcode = LIBROLE_OK;

    if (NULL == directory || NULL == filename || NULL == role_graph)
    {
        return retcode;
    }

    size_t dirlen = strlen(directory);
    size_t namelen = strlen(filename);
    size_t fullpathlen = dirlen + namelen + 1 + 1;

    if (fullpathlen > MAX_PATH_LEN)
    {
        return ENAMETOOLONG;
    }
    char fullpath[fullpathlen];

    /* Build full path to the file being read for roles */
    strcpy(fullpath, directory);
    strcat(fullpath, "/");
    strcat(fullpath, filename);

    /* Ignore return code from librole_reading to retain previous
     * (without /etc/role.d) behavior. */
    retcode = librole_reading(fullpath, role_graph);

    return retcode;
}

/**
 * \brief Read role entries from /etc/role.d catalog.
 *
 * \param[in] directory directory to open.
 * \return
 *  - LIBROLE_INCORRECT_VALUE: Incorrect values passed to function
 *  - LIBROLE_ERROR_OPENING_DIRECTORY: Error occured while trying to
 *    open directory.
 *  - LIBROLE_OK: Operation succeeded.
 *  - errno: Undefined error.
 */
int librole_get_directory_files(const char const *directory,
    struct librole_graph *role_graph)
{
    int retcode = LIBROLE_OK;

    if (NULL == directory || NULL == role_graph)
    {
        retcode = LIBROLE_INCORRECT_VALUE;
        goto librole_get_directory_files_end;
    }

    errno = 0;

    struct dirent **files;
    int file_count = 0;
    DIR *role_catalog = NULL;

    /* Get all regular files in directory */
    file_count = scandir(directory, &files, librole_is_file, alphasort);
    if (0 != errno)
    {
        retcode = errno;
        goto librole_get_directory_files_end;
    }

    int i = 0;
    for (i = 0; i < file_count; ++i)
    {
        /* Don't do anything on errors and try to continue reading */
        retcode = librole_read_file_from_dir(
            directory, files[i]->d_name, role_graph);
        free(files[i]);
    }
    free(files);

librole_get_directory_files_end:
    return retcode;
}
