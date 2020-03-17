#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <grp.h>
#include <pwd.h>

#include <sys/types.h>

#include "role/parser.h"

/* how many times try double buffer */
#define BUFFERCOUNT 100

/* convert errno to librule result */
static int errno_to_result(int err)
{
    switch (err) {
        case ENOENT:
            return ENOENT;
        case ERANGE:
            return LIBROLE_OUT_OF_RANGE;
        case EMFILE:
        case ENFILE:
            return LIBROLE_NOFILE_ERROR;
        case EIO:
            return LIBROLE_IO_ERROR;
    }
    return LIBROLE_UNKNOWN_ERROR;
}

/* _SC_GETGR_R_SIZE_MAX
   _SC_GETPW_R_SIZE_MAX */
static void *get_buffer(int const_name, size_t *actual_size)
{
    void *buf;
    size_t bufsize = sysconf(const_name);
    if (bufsize == -1) {
        bufsize = 16384;
    }
    buf = malloc(bufsize);
    if (buf)
        *actual_size = bufsize;
    return buf;
}

/* more buffer size (new size will in size) */
int librole_realloc_buffer(void **buffer, size_t *size)
{
    size_t newsize = 2 * (*size);
    void *newbuffer;
    if (!buffer || !size)
        return LIBROLE_INTERNAL_ERROR;
#if DEBUG
    fprintf(stderr,"Realloc %p to %u\n", buffer, newsize);
#endif
    newbuffer = realloc(*buffer, newsize * sizeof(char));
    if (!newbuffer) {
        free(buffer);
        *buffer = NULL;
        *size = 0;
        return LIBROLE_MEMORY_ERROR;
    }

    *buffer = newbuffer;
    *size = newsize;
    return LIBROLE_OK;
}

int librole_en_vector(void **buffer, size_t *capacity, size_t used, size_t elsize)
{
    size_t newsize;
    void *newbuffer;

    if (!buffer || !capacity)
        return LIBROLE_INTERNAL_ERROR;

    if (used != *capacity)
        return LIBROLE_OK;

    if (used > *capacity)
        return LIBROLE_INTERNAL_ERROR;

    newsize = (*capacity) * 2;

    newbuffer = realloc(*buffer, newsize * elsize);
    if (!newbuffer) {
        //free(buffer);
        //*buffer = NULL;
        //*capacity = 0;
        return LIBROLE_MEMORY_ERROR;
    }

    *buffer = newbuffer;
    *capacity = newsize;
    return LIBROLE_OK;
}

static int str_to_gid(const char *gr_name, gid_t *result_gid)
{
    gid_t gid;
    char *p;
    gid = (gid_t)strtoul(gr_name, &p, 10);

    /* if there is gid in gr_name (all digits or '0') */
    if (*p == '\0' && (gid != 0 || gr_name[0] == '0')) {
        /* no result_gid pointer check */
        *result_gid = gid;
        return 1;
    }
    return 0;
}

/* check gid and get group name by gid */
int librole_get_group_name(gid_t gid, char *ans, size_t ans_size)
{
    struct group grp, *grp_ptr;
    void *buffer;
    size_t bufsize;
    int err, result;

    buffer = get_buffer(_SC_GETGR_R_SIZE_MAX, &bufsize);
    if (!buffer)
        return LIBROLE_MEMORY_ERROR;

    for (int i = 0 ; i < BUFFERCOUNT ; i++) {
        err = getgrgid_r(gid, &grp, buffer, bufsize, &grp_ptr);
        if (err != ERANGE)
            break;
        result = librole_realloc_buffer((void**)&buffer, &bufsize);
        if (result != LIBROLE_OK)
            return result;
    }

    if  (err != 0) {
        free(buffer);
        return errno_to_result(err);
    }
    if (!grp_ptr) {
        free(buffer);
        return LIBROLE_NO_SUCH_GROUP;
    }
    if (ans) {
        strncpy(ans, grp.gr_name, ans_size);
        ans[ans_size] = '\0';
    }

    free(buffer);
    return LIBROLE_OK;
}

/* get gid by group name */
static int get_gid_by_groupname(const char *gr_name, gid_t *gid)
{
    struct group grp, *grp_ptr;
    void *buffer;
    size_t bufsize;
    int err, result;

    buffer = get_buffer(_SC_GETGR_R_SIZE_MAX, &bufsize);
    if (!buffer)
        return LIBROLE_MEMORY_ERROR;

    for (int i = 0 ; i < BUFFERCOUNT ; i++) {
        err = getgrnam_r(gr_name, &grp, buffer, bufsize, &grp_ptr);
        if (err != ERANGE)
            break;
        result = librole_realloc_buffer(&buffer, &bufsize);
        if (result != LIBROLE_OK)
            return result;
    }

    if  (err != 0) {
        free(buffer);
        return errno_to_result(err);
    }
    if (!grp_ptr) {
        free(buffer);
        return LIBROLE_NO_SUCH_GROUP;
    }
    if (gid) {
        *gid = grp.gr_gid;
    }

    free(buffer);
    return LIBROLE_OK;
}

static int check_group_name(const char *str)
{
    size_t len = strlen(str);

    if (!len)
        return LIBROLE_INCORRECT_VALUE;

    /* if there are quotes */
    if (str[0] == '"' || str[len-1] == '"')
        return LIBROLE_INCORRECT_VALUE;

    return LIBROLE_OK;
}

/* get real gid by group name or gid */
int librole_get_gid(const char *gr_name, gid_t *ans)
{
    gid_t gid;
    int result;

    result = check_group_name(gr_name);
    if (result != LIBROLE_OK)
        return result;

    if (str_to_gid(gr_name, &gid))
        result = librole_get_group_name(gid, NULL, 0);
    else
        result = get_gid_by_groupname(gr_name, &gid);

    if ((result == LIBROLE_OK) && ans)
        *ans = gid;

    return result;
}


/* get username by uid */
int librole_get_user_name(uid_t uid, char *user_name, size_t user_name_size)
{
    size_t bufsize;
    void *buffer;
    struct passwd pwd;
    struct passwd* pwd_ptr;
    int err, result;

    buffer = get_buffer(_SC_GETPW_R_SIZE_MAX, &bufsize);
    if (!buffer)
    return LIBROLE_MEMORY_ERROR;

    for (int i = 0 ; i < BUFFERCOUNT ; i++) {
        err = getpwuid_r(uid, &pwd, buffer, bufsize, &pwd_ptr);
        if (err != ERANGE)
            break;
        result = librole_realloc_buffer(&buffer, &bufsize);
        if (result != LIBROLE_OK)
            return result;
    }

    if  (err != 0) {
        free(buffer);
        return errno_to_result(err);
    }

    if (!pwd_ptr) {
        free(buffer);
        return LIBROLE_NO_SUCH_GROUP;
    }

    if (user_name) {
        strncpy(user_name, pwd_ptr->pw_name, user_name_size);
        user_name[user_name_size - 1 ] = '\0';
    }

    free(buffer);
    return LIBROLE_OK;
}

void librole_print_error(int result)
{
    const char *errtext = "Unknown error (missed in librole_print_error switch)";

    if (!result)
        return;

    switch (result) {
        case LIBROLE_IO_ERROR:
            errtext = "I/O file error";
            break;
        case LIBROLE_MEMORY_ERROR:
            errtext = "Library bug: Internal memory error";
            break;
        case LIBROLE_OUT_OF_RANGE:
            errtext = "Have no enough memory";
            break;
        case LIBROLE_NO_SUCH_GROUP:
            errtext = "No such group";
            break;
        case LIBROLE_UNKNOWN_ERROR:
            errtext = "Unknown error";
            break;
        case LIBROLE_NOFILE_ERROR:
            errtext = "Have no free file descriptors";
            break;
        case LIBROLE_INTERNAL_ERROR:
            errtext = "Internal library error (program bug)";
            break;
        case LIBROLE_PAM_ERROR:
            errtext = "PAM error";
            break;
        case LIBROLE_INCORRECT_VALUE:
            errtext = "Incorrect value";
            break;
        case LIBROLE_OK:
            errtext = "No error";
            break;
        default:
            errtext = strerror(result);
            break;
    }
    fprintf(stderr, "Error %d: %s\n", result, errtext);
}

int librole_create_ver_from_args(int argc, char **argv, int optind, struct librole_ver *new_role, int skip_flag)
{
    int result;
    result = librole_ver_init(new_role);
    if (result != LIBROLE_OK)
        return result;

    result = librole_get_gid(argv[optind++], &new_role->gid);
    if (result != LIBROLE_OK)
        return result;

    while (optind < argc) {
        gid_t gid;
        result = librole_get_gid(argv[optind++], &gid);
        if (result == LIBROLE_NO_SUCH_GROUP) {
            if (skip_flag)
                continue;
            fprintf(stderr,"No such group: %s!\n", argv[optind-1]);
            return result;
        }
        if (result != LIBROLE_OK)
            return result;

        result = librole_ver_add(new_role, gid);
        if (result != LIBROLE_OK)
            return result;
    }
    return LIBROLE_OK;
}
