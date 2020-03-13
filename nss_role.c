#include <nss.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "role/parser.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

enum nss_status _nss_role_initgroups_dyn(char *user, gid_t main_group,
        long int *start, long int *size, gid_t **groups,
        long int limit, int *errnop)
{
    enum nss_status ret = NSS_STATUS_SUCCESS;
    pthread_mutex_lock(&mutex);

    struct librole_graph G = {0};
    int i, result;
    librole_group_collector col = {0}, ans = {0};

    result = librole_graph_init(&G);
    if (result != LIBROLE_OK) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

    result = librole_reading(LIBROLE_CONFIG, &G);
    if (result != LIBROLE_OK) {
        if (result == LIBROLE_MEMORY_ERROR) {
            *errnop = ENOMEM;
            ret =  NSS_STATUS_NOTFOUND;
        } else
            ret = NSS_STATUS_UNAVAIL;
        goto libnss_role_out;
    }

    result = librole_ver_init(&col);
    if (result != LIBROLE_OK) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

    memset(G.used, 0, sizeof(int) * G.capacity);
    result = librole_dfs(&G, main_group, &col);
    if (result == LIBROLE_MEMORY_ERROR) {
        *errnop = ENOMEM;
        ret = NSS_STATUS_NOTFOUND;
        goto libnss_role_out;
    }

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
    free(ans.list);
    free(col.list);
    librole_graph_free(&G);
    pthread_mutex_unlock(&mutex);
    return ret;
}
