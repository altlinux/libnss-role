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
#include <string.h>

#include "test_config.h"
#include "test_memory.h"
#include "role/fileop.h"
#include "role/parser.h"
#include "role/glob.h"

/* gids from data/group */
#define GID_USERS 100
#define GID_VIDEO 482
#define GID_WHEEL 5
#define GID_TFTP 40
#define GID_AUDIO 50

static int has_gid(const struct librole_ver *v, gid_t gid)
{
    return librole_ver_find_gid((struct librole_ver *) v, gid, NULL) == LIBROLE_OK;
}

/* The name must fit with '\0', nothing is written past the buffer */
void test_get_group_name_buffer(void **state)
{
    char buf[16];

    (void) state;
    memset(buf, 'X', sizeof(buf));
    assert_int_equal(librole_get_group_name(GID_VIDEO, buf, 6), LIBROLE_OK);
    assert_string_equal(buf, "video");
    assert_int_equal(buf[6], 'X');

    /* Don't return a truncated name */
    memset(buf, 'X', sizeof(buf));
    assert_int_equal(librole_get_group_name(GID_VIDEO, buf, 5),
                     LIBROLE_OUT_OF_RANGE);
    assert_int_equal(buf[5], 'X');
}

/* Privileges can be added to a role dropped from the graph */
void test_role_drop_then_add(void **state)
{
    struct librole_graph G;
    struct librole_ver role;
    gid_t gids[] = { GID_VIDEO, GID_WHEEL, GID_TFTP, GID_AUDIO };
    int i, idx;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_reading(__LIBROLE_TEST_DATADIR "/role_file" LIBROLE_ROLE_EXTENSION, &G),
                     LIBROLE_OK);

    role.gid = GID_USERS;
    role.list = NULL;
    role.size = role.capacity = 0;
    assert_int_equal(librole_role_drop(&G, role), LIBROLE_OK);

    assert_int_equal(librole_ver_init(&role), LIBROLE_OK);
    role.gid = GID_USERS;
    for (i = 0; i < 4; i++)
        assert_int_equal(librole_ver_add(&role, gids[i]), LIBROLE_OK);
    assert_int_equal(librole_role_add(&G, role), LIBROLE_OK);

    assert_int_equal(librole_find_gid(&G, GID_USERS, &idx), LIBROLE_OK);
    assert_int_equal(G.gr[idx].size, 4);
    for (i = 0; i < 4; i++)
        assert_true(has_gid(&G.gr[idx], gids[i]));

    librole_ver_free(&role);
    librole_graph_free(&G);
}

/* Graph grows and marks new roles as not visited */
void test_graph_grow(void **state)
{
    struct librole_graph G;
    struct librole_ver role;
    int i;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    for (i = 0; i < 25; i++) {
        assert_int_equal(librole_ver_init(&role), LIBROLE_OK);
        role.gid = 1000 + i;
        assert_int_equal(librole_graph_add(&G, role), LIBROLE_OK);
    }
    assert_int_equal(G.size, 25);
    assert_true(G.capacity >= 25);
    for (i = 0; i < G.capacity; i++)
        assert_int_equal(G.used[i], 0);

    librole_graph_free(&G);
}

void test_validate_filename(void **state)
{
    (void) state;
    assert_int_equal(librole_validate_filename_from_dir("x" LIBROLE_ROLE_EXTENSION),
                     LIBROLE_OK);
    assert_int_equal(librole_validate_filename_from_dir("a"),
                     LIBROLE_INVALID_ROLE_FILENAME);
    assert_int_equal(librole_validate_filename_from_dir("role"),
                     LIBROLE_INVALID_ROLE_FILENAME);
    assert_int_equal(librole_validate_filename_from_dir(LIBROLE_ROLE_EXTENSION),
                     LIBROLE_INVALID_ROLE_FILENAME);
    assert_int_equal(librole_validate_filename_from_dir("x.rol"),
                     LIBROLE_INVALID_ROLE_FILENAME);
}
