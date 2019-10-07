#include <nss.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "role/parser.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int librole_realloc_groups(long int **size, gid_t ***groups, long int new_size)
{
    gid_t *new_groups;

    new_groups = (gid_t *)
        realloc((**groups),
            new_size * sizeof(***groups));

    if (!new_groups)
        return LIBROLE_MEMORY_ERROR;

    **groups = new_groups;
    **size = new_size;

    return LIBROLE_OK;
}

enum nss_status _nss_role_initgroups_dyn(char *user, gid_t main_group,
        long int *start, long int *size, gid_t **groups,
        long int limit, int *errnop)
{
    enum nss_status ret = NSS_STATUS_SUCCESS;
    pthread_mutex_lock(&mutex);

    struct librole_graph G;
    int i, result;
    struct librole_ver col, ans;

    result = librole_reading("/etc/role", &G);
    if (result != LIBROLE_OK) {
        if (result == LIBROLE_MEMORY_ERROR) {
            *errnop = ENOMEM;
            ret =  NSS_STATUS_NOTFOUND;
        } else
            ret = NSS_STATUS_UNAVAIL;
        goto libnss_role_out;
    }

    result = librole_graph_init(&G);
    if (result != LIBROLE_OK) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

    result = librole_ver_init(&col);
    if (result != LIBROLE_OK) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

    /* put all groups and privileges to the col list */
    /* add privileges for the main group */
    result = librole_dfs(&G, main_group, &col);
    if (result == LIBROLE_MEMORY_ERROR) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

    /* add privileges for second groups */
    for(i = 0; i < *start; i++) {
        result = librole_dfs(&G, (*groups)[i], &col);
        if (result == LIBROLE_MEMORY_ERROR) {
            *errnop = ENOMEM;
            ret = NSS_STATUS_NOTFOUND;
            goto libnss_role_out;
        }
    }

    result = librole_ver_init(&ans);
    if (result != LIBROLE_OK) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

/* FIXME: drop that code */
/* Description: some kind of deduplication:
   add to ans list all item from col which not in groups and not in ans already */
    for(i = 0; i < col.size; i++) {
        int exists = 0, j;
        for(j = 0; j < *start; j++) {
            if ((*groups)[j] == col.list[i]) {
                exists = 1;
                break;
            }
        }
        if (main_group == col.list[i])
            exists = 1;
        for(j = 0; j < ans.size; j++) {
            if (ans.list[j] == col.list[i]) {
                exists = 1;
                break;
            }
        }

        if (exists)
            continue;

        result = librole_ver_add(&ans, col.list[i]);
        if (result != LIBROLE_OK) {
            *errnop = ENOMEM;
            ret = NSS_STATUS_NOTFOUND;
            goto libnss_role_out;
        }
    }

    if (*start + ans.size > *size) {
        if ((limit >= 0 && *start + ans.size > limit) ||
            librole_realloc_groups(&size, &groups,
                *start + ans.size) != LIBROLE_OK) {
            *errnop = ENOMEM;
            ret = NSS_STATUS_NOTFOUND;
            goto libnss_role_out;
        }
    }

    for(i = 0; i < ans.size; i++)
        (*groups)[(*start)++] = ans.list[i];

libnss_role_out:
//TODO: rewrite all places with free on exit
    librole_ver_free(&ans);
    librole_ver_free(&col);
    librole_graph_free(&G);
    pthread_mutex_unlock(&mutex);
    return ret;
}
