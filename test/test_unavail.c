/*
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
#define _GNU_SOURCE

/* cmocka requirements */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/* Test requirements */
#include <dlfcn.h>
#include <errno.h>
#include <grp.h>
#include <nss.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "role/fileop.h"

#include "test_config.h"
#include "test_unavail.h"
#include "role/parser.h"
#include "role/glob.h"

/*
 * Groups which live in a group database that is not available, like
 * winbind before winbindd is started: glibc returns the errno of the
 * NSS module (ENOENT) from getgrnam_r()/getgrgid_r().
 * All other lookups are passed to the next definition (nss_wrapper).
 * nss_wrapper returns ENOENT for a missing group (and leaves *result
 * untouched), while glibc returns 0 and sets *result to NULL: convert it
 * to glibc behaviour, so a missing group is not taken for unavailable one.
 */
#define UNAVAIL_GROUP_NAME "dom_unavail"
#define UNAVAIL_GROUP_GID 20000

/* gids from data/group */
#define GID_USERS 100
#define GID_VIDEO 482
#define GID_WHEEL 5
#define GID_TFTP 40
#define GID_AUDIO 50

static const char *role_file_unavail = __LIBROLE_TEST_DATADIR "/role.test.unavail";
static const char *role_file_no_eol = __LIBROLE_TEST_DATADIR "/role.test.no_eol";
static const char *role_dir_missing = __LIBROLE_TEST_DATADIR "/role.test.missing.d";

static int glibc_result(int ret, struct group **result)
{
    if (ret == ENOENT) {
        *result = NULL;
        return 0;
    }
    return ret;
}

/* ISO C doesn't allow to convert void * from dlsym() to function pointer */
union getgrnam_r_ptr {
    void *ptr;
    int (*fn)(const char *, struct group *, char *, size_t, struct group **);
};

union getgrgid_r_ptr {
    void *ptr;
    int (*fn)(gid_t, struct group *, char *, size_t, struct group **);
};

int getgrnam_r(const char *name, struct group *grp, char *buf,
               size_t buflen, struct group **result)
{
    union getgrnam_r_ptr real;

    if (!strcmp(name, UNAVAIL_GROUP_NAME)) {
        *result = NULL;
        return ENOENT;
    }
    real.ptr = dlsym(RTLD_NEXT, "getgrnam_r");
    return glibc_result(real.fn(name, grp, buf, buflen, result), result);
}

int getgrgid_r(gid_t gid, struct group *grp, char *buf,
               size_t buflen, struct group **result)
{
    union getgrgid_r_ptr real;

    if (gid == UNAVAIL_GROUP_GID) {
        *result = NULL;
        return ENOENT;
    }
    real.ptr = dlsym(RTLD_NEXT, "getgrgid_r");
    return glibc_result(real.fn(gid, grp, buf, buflen, result), result);
}

/* librole_config_dir() is mocked in test_paths.c */
const char *librole_config_file(void) {
    return mock_ptr_type(const char *);
}

enum nss_status _nss_role_initgroups_dyn(char *user, gid_t main_group,
        long int *start, long int *size, gid_t **groups,
        long int limit, int *errnop);

static void write_file(const char *filename, const char *content)
{
    FILE *f = fopen(filename, "w");

    assert_non_null(f);
    assert_int_equal(fputs(content, f) < 0, 0);
    assert_int_equal(fclose(f), 0);
}

static int parse_string(const char *line, struct librole_graph *G)
{
    char *mutable_line = strdup(line);
    int result;

    assert_non_null(mutable_line);
    result = parse_line(mutable_line, G);
    free(mutable_line);

    return result;
}

static struct librole_ver *find_role(struct librole_graph *G, gid_t gid)
{
    int idx;

    if (librole_find_gid(G, gid, &idx) != LIBROLE_OK)
        return NULL;
    return &G->gr[idx];
}

static int has_gid(const gid_t *list, long int size, gid_t gid)
{
    long int i;

    for (i = 0; i < size; i++)
        if (list[i] == gid)
            return 1;
    return 0;
}

/* Error on a line extending an existing role must not free its list:
 * it is owned by the graph (double free in librole_graph_free()) */
void test_parse_line_existing_role_error(void **state)
{
    struct librole_graph G;
    struct librole_ver *role;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);

    assert_int_equal(parse_string("users:video", &G), LIBROLE_OK);
    assert_int_equal(parse_string("users:wheel,,audio", &G),
                     LIBROLE_INCORRECT_VALUE);

    role = find_role(&G, GID_USERS);
    assert_non_null(role);
    assert_true(has_gid(role->list, role->size, GID_VIDEO));
    assert_true(has_gid(role->list, role->size, GID_WHEEL));

    librole_graph_free(&G);
}

void test_parse_line_unavail_member(void **state)
{
    struct librole_graph G;
    struct librole_ver *role;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);

    assert_int_equal(parse_string("users:video," UNAVAIL_GROUP_NAME ",20000,wheel", &G),
                     LIBROLE_SOURCE_UNAVAIL);

    role = find_role(&G, GID_USERS);
    assert_non_null(role);
    assert_int_equal(role->size, 2);
    assert_true(has_gid(role->list, role->size, GID_VIDEO));
    assert_true(has_gid(role->list, role->size, GID_WHEEL));

    librole_graph_free(&G);
}

void test_parse_line_unavail_role(void **state)
{
    struct librole_graph G;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);

    assert_int_equal(parse_string(UNAVAIL_GROUP_NAME ":wheel", &G),
                     LIBROLE_SOURCE_UNAVAIL);
    assert_int_equal(parse_string("20000:wheel", &G),
                     LIBROLE_SOURCE_UNAVAIL);
    assert_int_equal(G.size, 0);

    librole_graph_free(&G);
}

/* The rest of the file is read, but the caller knows it is incomplete */
void test_reading_unavail(void **state)
{
    struct librole_graph G;
    struct librole_ver *role;

    (void) state;
    write_file(role_file_unavail,
               UNAVAIL_GROUP_NAME ":wheel\n"
               "users:video,20000\n"
               "audio:tftp\n");
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);

    assert_int_equal(librole_reading(role_file_unavail, &G),
                     LIBROLE_SOURCE_UNAVAIL);
    assert_int_equal(G.size, 2);
    role = find_role(&G, GID_USERS);
    assert_non_null(role);
    assert_int_equal(role->size, 1);
    assert_int_equal(role->list[0], GID_VIDEO);
    role = find_role(&G, GID_AUDIO);
    assert_non_null(role);
    assert_int_equal(role->list[0], GID_TFTP);

    librole_graph_free(&G);
    unlink(role_file_unavail);
}

/* The last line without '\n' is handled like any other line */
void test_reading_last_line_without_eol(void **state)
{
    struct librole_graph G;

    (void) state;
    write_file(role_file_no_eol,
               "users:video\n"
               "nosuchrole:wheel");
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);

    assert_int_equal(librole_reading(role_file_no_eol, &G), LIBROLE_OK);
    assert_int_equal(G.size, 1);
    assert_non_null(find_role(&G, GID_USERS));

    librole_graph_free(&G);
    unlink(role_file_no_eol);
}

/* NSS module returns roles which can be resolved instead of failing */
void test_nss_initgroups_unavail(void **state)
{
    long int start = 1, size = 1;
    gid_t *groups = malloc(sizeof(gid_t));
    int err = 0;

    (void) state;
    assert_non_null(groups);
    groups[0] = GID_USERS;
    write_file(role_file_unavail,
               UNAVAIL_GROUP_NAME ":wheel\n"
               "users:video,20000\n"
               "video:audio\n");
    will_return(librole_config_file, role_file_unavail);
    will_return(librole_config_dir, role_dir_missing);

    assert_int_equal(_nss_role_initgroups_dyn("user", GID_USERS,
                         &start, &size, &groups, -1, &err),
                     NSS_STATUS_SUCCESS);
    assert_int_equal(start, 3);
    assert_true(has_gid(groups, start, GID_VIDEO));
    assert_true(has_gid(groups, start, GID_AUDIO));
    assert_false(has_gid(groups, start, GID_WHEEL));

    free(groups);
    unlink(role_file_unavail);
}

void test_reading_directory_unavail(void **state)
{
    struct librole_graph G;
    const char *dir = __LIBROLE_TEST_DATADIR "/role.test.unavail.d";
    const char *first = __LIBROLE_TEST_DATADIR "/role.test.unavail.d/a.role";
    const char *last = __LIBROLE_TEST_DATADIR "/role.test.unavail.d/z.role";

    (void) state;
    assert_int_equal(mkdir(dir, 0700), 0);
    write_file(first, "users:dom_unavail,video\n");
    write_file(last, "audio:tftp\n");
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_get_directory_files(dir, &G), LIBROLE_SOURCE_UNAVAIL);
    assert_int_equal(G.size, 2);
    librole_graph_free(&G);
    unlink(first);
    unlink(last);
    rmdir(dir);
}
