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
#include "test_files.h"
#include "role/parser.h"
#include "role/fileop.h"
#include "role/glob.h"

/* gids from data/group */
#define GID_USERS 100
#define GID_VIDEO 482
#define GID_WHEEL 5
#define GID_AUDIO 50

static const char *role_file_missing = __LIBROLE_TEST_DATADIR "/role.test.missing";
static const char *role_dir = __LIBROLE_TEST_DATADIR "/role.test.files.d";
static const char *role_dir_a = __LIBROLE_TEST_DATADIR "/role.test.files.d/a" LIBROLE_ROLE_EXTENSION;
static const char *role_dir_b = __LIBROLE_TEST_DATADIR "/role.test.files.d/b" LIBROLE_ROLE_EXTENSION;

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

int files_test_setup(void **state)
{
    (void) state;
    unlink(role_file_missing);
    mkdir(role_dir, 0755);
    write_file(role_dir_a, "audio:tftp\n");
    write_file(role_dir_b, "users:video\n");
    return 0;
}

int files_test_teardown(void **state)
{
    (void) state;
    unlink(role_dir_a);
    unlink(role_dir_b);
    rmdir(role_dir);
    return 0;
}

/* A missing file is an empty configuration, other errors are reported */
void test_reading_missing_file(void **state)
{
    struct librole_graph G;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_reading(role_file_missing, &G), LIBROLE_OK);
    assert_int_equal(G.size, 0);
    /* ENOTDIR */
    assert_int_equal(librole_reading(__LIBROLE_TEST_DATADIR "/group/role", &G),
                     LIBROLE_IO_ERROR);
    librole_graph_free(&G);
}

/* NSS module reads /etc/role.d without /etc/role */
void test_nss_initgroups_missing_file(void **state)
{
    long int start = 1, size = 1;
    gid_t *groups = malloc(sizeof(gid_t));
    int err = 0;

    (void) state;
    assert_non_null(groups);
    groups[0] = GID_USERS;
    will_return(librole_config_file, role_file_missing);
    will_return(librole_config_dir, role_dir);

    assert_int_equal(_nss_role_initgroups_dyn("user", GID_USERS,
                         &start, &size, &groups, -1, &err),
                     NSS_STATUS_SUCCESS);
    assert_int_equal(start, 2);
    assert_int_equal(groups[1], GID_VIDEO);

    free(groups);
}

void test_find_role_file(void **state)
{
    char *filename = NULL;

    (void) state;
    assert_int_equal(librole_find_role_file(role_dir, GID_USERS, &filename),
                     LIBROLE_OK);
    assert_string_equal(filename, "b" LIBROLE_ROLE_EXTENSION);
    free(filename);

    assert_int_equal(librole_find_role_file(role_dir, GID_WHEEL, &filename),
                     LIBROLE_NO_SUCH_GROUP);
    assert_null(filename);
    assert_int_equal(librole_find_role_file(__LIBROLE_TEST_DATADIR "/role.test.no.d",
                                            GID_USERS, &filename),
                     LIBROLE_NO_SUCH_GROUP);
}
