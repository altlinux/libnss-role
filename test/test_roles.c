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
/* cmocka requirements */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/* Test requirements */
#include <nss.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test_config.h"
#include "test_roles.h"
#include "role/parser.h"
#include "role/fileop.h"
#include "role/glob.h"

/* gids from data/group */
#define GID_USERS 100
#define GID_VIDEO 482
#define GID_WHEEL 5
#define GID_TFTP 40
#define GID_AUDIO 50

static const char *role_file = __LIBROLE_TEST_DATADIR "/role.test.roles";
static const char *role_dir = __LIBROLE_TEST_DATADIR "/role.test.d";
static const char *role_dir_file = __LIBROLE_TEST_DATADIR "/role.test.d/extra" LIBROLE_ROLE_EXTENSION;
static const char *role_dir_other = __LIBROLE_TEST_DATADIR "/role.test.d/extra.txt";

/* librole_config_file() and librole_config_dir() are mocked in
 * test_unavail.c and test_paths.c */

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

static void parse_string(const char *line, struct librole_graph *G)
{
    char *mutable_line = strdup(line);

    assert_non_null(mutable_line);
    parse_line(mutable_line, G);
    free(mutable_line);
}

static struct librole_ver *find_role(struct librole_graph *G, gid_t gid)
{
    int idx;

    if (librole_find_gid(G, gid, &idx) != LIBROLE_OK)
        return NULL;
    return &G->gr[idx];
}

static int count_gid(const gid_t *list, long int size, gid_t gid)
{
    long int i;
    int count = 0;

    for (i = 0; i < size; i++)
        if (list[i] == gid)
            count++;
    return count;
}

static void assert_role(struct librole_graph *G, gid_t gid,
                        const gid_t *privs, int count)
{
    struct librole_ver *role = find_role(G, gid);
    int i;

    assert_non_null(role);
    assert_int_equal(role->size, count);
    for (i = 0; i < count; i++)
        assert_int_equal(count_gid(role->list, role->size, privs[i]), 1);
}

int roles_test_setup(void **state)
{
    (void) state;
    mkdir(role_dir, 0755);
    write_file(role_file,
               "users:video\n"
               "video:audio\n");
    write_file(role_dir_file, "audio:tftp\n");
    /* Not a role file: ignored */
    write_file(role_dir_other, "audio:wheel\n");
    return 0;
}

int roles_test_teardown(void **state)
{
    (void) state;
    unlink(role_file);
    unlink(role_dir_file);
    unlink(role_dir_other);
    rmdir(role_dir);
    return 0;
}

/* Comments, spaces, quotes, numeric gids and missing groups */
void test_parse_line_formats(void **state)
{
    struct librole_graph G;
    const gid_t users_privs[] = { GID_VIDEO, GID_WHEEL, GID_AUDIO };
    const gid_t wheel_privs[] = { GID_TFTP, GID_AUDIO };
    const gid_t tftp_privs[] = { GID_AUDIO };

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);

    parse_string("# comment: wheel", &G);
    parse_string("", &G);
    assert_int_equal(G.size, 0);

    parse_string("users:  video , wheel  # comment", &G);
    /* The same role on another line is merged */
    parse_string("\"users\":\"audio\",nosuchgroup", &G);
    parse_string("5:40,50", &G);
    parse_string("tftp:nosuchgroup,audio", &G);
    parse_string("nosuchrole:wheel", &G);

    assert_int_equal(G.size, 3);
    assert_role(&G, GID_USERS, users_privs, 3);
    assert_role(&G, GID_WHEEL, wheel_privs, 2);
    assert_role(&G, GID_TFTP, tftp_privs, 1);

    librole_graph_free(&G);
}

/* Only *.role files from the directory are read and merged */
void test_reading_role_dir(void **state)
{
    struct librole_graph G;
    const gid_t audio_privs[] = { GID_TFTP };

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_reading(role_file, &G), LIBROLE_OK);
    assert_int_equal(librole_get_directory_files(role_dir, &G), LIBROLE_OK);

    assert_int_equal(G.size, 3);
    assert_role(&G, GID_AUDIO, audio_privs, 1);

    librole_graph_free(&G);
}

/* Roles are resolved transitively, cycles terminate */
void test_dfs_nested_and_cycle(void **state)
{
    struct librole_graph G;
    librole_group_collector col;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    parse_string("users:video", &G);
    parse_string("video:wheel", &G);
    parse_string("wheel:users", &G);
    assert_int_equal(librole_ver_init(&col), LIBROLE_OK);

    assert_int_equal(librole_dfs(&G, GID_USERS, &col), LIBROLE_OK);
    assert_int_equal(col.size, 3);
    assert_int_equal(count_gid(col.list, col.size, GID_USERS), 1);
    assert_int_equal(count_gid(col.list, col.size, GID_VIDEO), 1);
    assert_int_equal(count_gid(col.list, col.size, GID_WHEEL), 1);

    librole_ver_free(&col);
    librole_graph_free(&G);
}

/* NSS module adds nested roles from /etc/role and /etc/role.d once */
void test_nss_initgroups(void **state)
{
    long int start = 1, size = 1;
    gid_t *groups = malloc(sizeof(gid_t));
    int err = 0;

    (void) state;
    assert_non_null(groups);
    groups[0] = GID_USERS;
    will_return(librole_config_file, role_file);
    will_return(librole_config_dir, role_dir);

    assert_int_equal(_nss_role_initgroups_dyn("user", GID_USERS,
                         &start, &size, &groups, -1, &err),
                     NSS_STATUS_SUCCESS);
    assert_int_equal(start, 4);
    assert_true(size >= start);
    assert_int_equal(count_gid(groups, start, GID_USERS), 1);
    assert_int_equal(count_gid(groups, start, GID_VIDEO), 1);
    assert_int_equal(count_gid(groups, start, GID_AUDIO), 1);
    assert_int_equal(count_gid(groups, start, GID_TFTP), 1);
    assert_int_equal(count_gid(groups, start, GID_WHEEL), 0);

    free(groups);
}

/* Groups which the user already has are not added again */
void test_nss_initgroups_existing_groups(void **state)
{
    long int start = 2, size = 2;
    gid_t *groups = malloc(2 * sizeof(gid_t));
    int err = 0;

    (void) state;
    assert_non_null(groups);
    groups[0] = GID_USERS;
    groups[1] = GID_AUDIO;
    will_return(librole_config_file, role_file);
    will_return(librole_config_dir, role_dir);

    assert_int_equal(_nss_role_initgroups_dyn("user", GID_USERS,
                         &start, &size, &groups, -1, &err),
                     NSS_STATUS_SUCCESS);
    assert_int_equal(start, 4);
    assert_int_equal(count_gid(groups, start, GID_AUDIO), 1);
    assert_int_equal(count_gid(groups, start, GID_VIDEO), 1);
    assert_int_equal(count_gid(groups, start, GID_TFTP), 1);

    free(groups);
}
