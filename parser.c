#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "role/parser.h"
#include "role/pam_check.h"
#include "role/lock_file.h"

// TODO: separate read/write and graph

int librole_graph_add(struct librole_graph *G, struct librole_ver v)
{
    if (G->size == G->capacity) {
        G->capacity <<= 1;
        G->gr = (struct librole_ver *) realloc(G->gr, sizeof(struct librole_ver) * G->capacity);
        if (!G->gr)
            return LIBROLE_MEMORY_ERROR;

        G->used = (int *) realloc(G->used, sizeof(int) * G->capacity);
        if (!G->used) {
            free(G->gr);
            return LIBROLE_MEMORY_ERROR;
        }
    }
    G->gr[G->size++] = v;
    return LIBROLE_OK;
}

int librole_ver_add(struct librole_ver *v, gid_t g)
{
    int privilege_index = 0;
    int result = LIBROLE_OK;

    result = librole_ver_find_gid(v, g, &privilege_index);
    /* GID 'g' for this role is already added */
    if (LIBROLE_OK == result) {
        goto librole_ver_add_end;
    }

    /* This code will be executed on LIBROLE_NO_SUCH_GROUP from
     * librole_ver_find_gid or on any other result */
    if (v->size == v->capacity) {
        v->capacity <<= 1;
        v->list = (gid_t *) realloc(v->list, sizeof(gid_t) * v->capacity);
        if (!v->list)
            return LIBROLE_MEMORY_ERROR;
    }
    v->list[v->size++] = g;

librole_ver_add_end:
    return LIBROLE_OK;
}

int librole_graph_init(struct librole_graph *G)
{
    G->capacity = 10;
    G->size = 0;
    G->gr = (struct librole_ver *) malloc(sizeof(struct librole_ver) * G->capacity);
    if (!G->gr)
        return LIBROLE_MEMORY_ERROR;

    G->used = (int *) malloc(sizeof(int) * G->capacity);
    if (!G->used) {
        free(G->gr);
        return LIBROLE_MEMORY_ERROR;
    }
    memset(G->used, 0, sizeof(int) * G->capacity);

    return LIBROLE_OK;
}

int librole_ver_init(struct librole_ver *v)
{
    v->capacity = 10;
    v->size = 0;
    v->gid = 0;
    v->list = (gid_t *) malloc(sizeof(gid_t) * v->capacity);
    if (!v->list)
        return LIBROLE_MEMORY_ERROR;

    return LIBROLE_OK;
}

int librole_find_gid(struct librole_graph *G, gid_t g, int *idx)
{
    int i;
    for(i = 0; i < G->size; i++) {
        if (G->gr[i].gid == g) {
            if (idx)
                *idx = i;
            return LIBROLE_OK;
        }
    }

    return LIBROLE_NO_SUCH_GROUP;
}

int librole_ver_find_gid(struct librole_ver* v, gid_t g, int *idx)
{
    int k;
    for(k = 0; k < v->size; k++)
        if (v->list[k] == g) {
            if (idx)
                *idx = k;
            return LIBROLE_OK;
        }
    return LIBROLE_NO_SUCH_GROUP;
}

void librole_ver_free(struct librole_ver *v)
{
    free(v->list);
    v->size = 0;
    v->capacity = 0;
}

void librole_graph_free(struct librole_graph *G)
{
    int i;

    for(i = 0; i < G->size; i++)
        librole_ver_free(&G->gr[i]);

    free(G->gr);
    free(G->used);
    G->size = 0;
    G->capacity = 0;
}

// TODO: move to internal (nss)
int librole_realloc_groups(long int **size, gid_t ***groups, long int new_size)
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

/* return 1 if finished on the start of the comment, otherwise - return 0 */
static int select_line_part(char *line, unsigned long len, char **last,
                 unsigned long *pos, const char symbol)
{
    unsigned long i = *pos;
    int in_progress = 0;
    int rc = 0;

    for(; i < len; i++) {
        if (!in_progress) {
            if (line[i] == ' ') {
                (*last)++;
                continue;
            } else {
                in_progress = 1;
            }
        }

        if (line[i] == '#') {
            line[i++] = '\0';
            rc = 1;
            break;
        }

        if (line[i] == symbol) {
            line[i++] = '\0';
            break;
        }
    }

    if (i > 1) {
        /* we have at least one symbol for a role name */
        unsigned long j = i - 1;

        while (j >= 1 && line[j - 1] == ' ')
            line[--j] = '\0';
    }

    *pos = i;
    return rc;
}

static void drop_quotes(char **str)
{
    size_t len = strlen(*str);
    if (*str[0] == '"' && *str[len-1] == '"') {
        *str[len-1] = '\0';
        *str++;
    }
}

static int parse_line(char *line, struct librole_graph *G)
{
    int result;
    unsigned long len = strlen(line);
    unsigned long i = 0;
    char *last = line;
    struct librole_ver default_role = {0, 0, 0, 10};
    struct librole_ver * role = &default_role;
    int comment = 0;
    int role_exists = 0;

    /* skip blank line */
    if (!len)
        return LIBROLE_OK;

    result = librole_ver_init(role);
    if (result != LIBROLE_OK)
        return result;

    comment = select_line_part(line, len, &last, &i, ':');

    if (comment && *last == '\0')
        goto libnss_role_parse_line_error;

    drop_quotes(&last);
    result = librole_get_gid(last, &role->gid);
    if (result != LIBROLE_OK)
        goto libnss_role_parse_line_error;

    int gid_index;
    result = librole_find_gid(G, role->gid, &gid_index);
    /* Use existing role if found */
    if (LIBROLE_OK == result) {
        free(role->list);
        role = &G->gr[gid_index];
        role_exists = 1;
    }

    while (!comment) {
        if (i >= len)
            break;
        last = line + i;
        gid_t gr;

        comment = select_line_part(line, len, &last, &i, ',');

        drop_quotes(&last);
        result = librole_get_gid(last, &gr);
        if (result == LIBROLE_NO_SUCH_GROUP)
            continue;
        if (result != LIBROLE_OK)
            goto libnss_role_parse_line_error;

        result = librole_ver_add(role, gr);
        if (result != LIBROLE_OK)
            goto libnss_role_parse_line_error;
    }

    if (0 == role_exists) {
        result = librole_graph_add(G, *role);
        if (result != LIBROLE_OK) {
            goto libnss_role_parse_line_error;
        }
    }
    return result;

libnss_role_parse_line_error:
    free(role->list);
    return result;
}

/*
 * \brief /etc/role file parser.
 *
 * \param[in] s path to file to read.
 * \param[in] G Role graph.
 *
 * \return
 *  - Status code
 */
int librole_reading(const char *s, struct librole_graph *G)
{
    int result = LIBROLE_OK;
    FILE *f = NULL;
    unsigned long len = LIBROLE_START_LINESIZE;
    char *str = NULL;
    unsigned long id = 0;
    int c;

    str = malloc(len * sizeof(char));
    if (!str)
        return LIBROLE_OUT_OF_RANGE;

    f = fopen(s, "r");
    if (!f) {
        result = LIBROLE_IO_ERROR;
        goto libnss_role_reading_out_free;
    }
    
    while(1) {
        c = fgetc(f);
        if (c == EOF)
            break;
        if (c == '\n') {
            str[id] = '\0';
            result = parse_line(str, G);
            if (result != LIBROLE_OK &&
                    result != LIBROLE_NO_SUCH_GROUP)
                goto libnss_role_reading_out;
            id = 0;
            result = LIBROLE_OK;
            continue;
        }
        str[id++] = c;
        if (id == len) {
            result = librole_realloc_buffer((void**)&str, &len);
            if (result != LIBROLE_OK)
                goto libnss_role_reading_out;
        }
    }
    if (id) {
        str[id] = '\0';
        result = parse_line(str, G);
        if (result != LIBROLE_OK)
            goto libnss_role_reading_out;
    }
    
libnss_role_reading_out:
    fclose(f);
libnss_role_reading_out_free:
    free(str);
    return result;
}

int librole_dfs(struct librole_graph *G, gid_t v, librole_group_collector *col)
{
    int i, j, result;

    result = librole_find_gid(G, v, &i);
    if (result != LIBROLE_OK) {
        result = librole_ver_add(col, v);
        return result;
    }

    if (G->used[i])
        return LIBROLE_OK;
    result = librole_ver_add(col, v);
    if (result != LIBROLE_OK)
        return result;
    G->used[i] = 1;

    for(j = 0; j < G->gr[i].size; j++) {
        result = librole_dfs(G, G->gr[i].list[j], col);
        if (result != LIBROLE_OK && result != LIBROLE_NO_SUCH_GROUP)
            return result;
    }
    return LIBROLE_OK;
}

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

// TODO: the same like in rolelst
// TODO: write in a new file and atomically rename
int librole_writing(const char *file, struct librole_graph *G, int numeric_flag)
{
    int i, j, result;
    FILE *f = fopen(file, "w");
    if (!f)
        return LIBROLE_IO_ERROR;

    for(i = 0; i < G->size; i++) {
        if (!G->gr[i].size)
            continue;

        result = write_group(f, G->gr[i].gid, -1, numeric_flag);
        if (result != LIBROLE_OK)
            goto libnss_role_writing_exit;

        for(j = 0; j < G->gr[i].size; j++) {
            result = write_group(f, G->gr[i].list[j], j, numeric_flag);
            if (result != LIBROLE_OK)
                goto libnss_role_writing_exit;
        }
        if (fputc('\n', f) < 0)
            goto libnss_role_writing_exit;
    }

    result = LIBROLE_OK;

libnss_role_writing_exit:
    fclose(f);
    return result;
}

int librole_write(const char* pam_role, struct librole_graph *G)
{
    int result;
    int pam_status;
    pam_handle_t *pamh;

    result = librole_pam_check(pamh, pam_role, &pam_status);
    if (result != LIBROLE_OK)
        return result;

    result = librole_lock(LIBROLE_CONFIG);
    if (result != LIBROLE_OK)
        goto exit;

    result = librole_writing(LIBROLE_CONFIG, G, 0);

    librole_unlock(LIBROLE_CONFIG);

// TODO: can we release immediately?
exit:
    librole_pam_release(pamh, pam_status);
    return result;
}
