/*
 * Copyright (c) 2008-2020 Etersoft
 * Copyright (c) 2020-2021 BaseALT Ltd. <org@basealt.ru>
 * Copyright (c) 2020-2021 Igor Chudov <nir@nir.org.ru>
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
/* cmocka requirements */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/* Test requirements */
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <grp.h>

#include "test_config.h"
#include "test_parser.h"
#include "role/fileop_rw.h"
#include "role/parser.h"
#include "role/glob.h"


static const char *file_role_test_string_new = __LIBROLE_TEST_DATADIR "/role.test.string.new" LIBROLE_ROLE_EXTENSION;
static const char *file_role_test_int_new = __LIBROLE_TEST_DATADIR "/role.test.int.new" LIBROLE_ROLE_EXTENSION;
static const char *file_role_test_add = __LIBROLE_TEST_DATADIR "/role.test.add" LIBROLE_ROLE_EXTENSION;
static const char *file_role_test_set = __LIBROLE_TEST_DATADIR "/role.test.set" LIBROLE_ROLE_EXTENSION;
static const char *file_role_test_del = __LIBROLE_TEST_DATADIR "/role.test.del" LIBROLE_ROLE_EXTENSION;
static const char *file_role_test_drop = __LIBROLE_TEST_DATADIR "/role.test.drop" LIBROLE_ROLE_EXTENSION;


static void assert_file_content(const char *filename, const char *content) {
    char buf[256];
    size_t len;
    FILE *f = fopen(filename, "r");

    assert_non_null(f);
    len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';
    assert_string_equal(buf, content);
}

static int unlink_test_file(const char *filename) {
    int result = 1;

    result = unlink(filename);
    if (ENOENT == errno || !result) {
        result = 0;
        goto unlink_test_file_end;
    }
    result = 1;

unlink_test_file_end:
    return result;
}


void test_drop_quotes(void **state) {
    char *orig_ptr = NULL;
    /* Create mutable memory area for test string */
    char *immutable_line[] = { "\"role_name\"" };
    size_t line_length = strlen(*immutable_line);
    char *mutable_line = calloc(line_length + 1, sizeof(char));
    (void) state;

    assert_non_null(mutable_line);
    /* drop_quotes increments the pointer making it impossible to free
     * the memory so it is needed to store the original pointer to
     * be able to free the memory block allocated */
    orig_ptr = mutable_line;
    strncpy(mutable_line, *immutable_line, line_length);
    strcpy(mutable_line, *immutable_line);

    assert_string_equal(mutable_line, "\"role_name\"");
    drop_quotes(&mutable_line);
    assert_string_equal(mutable_line, "role_name");

    free(orig_ptr);
    mutable_line = NULL;
    orig_ptr = NULL;
}

void test_parse_line(void **state) {
    struct librole_graph G;
    char *immutable_line[] = { "users:\"tftp\",named" };
    size_t line_length = strlen(*immutable_line);
    char *mutable_line = calloc(line_length + 1, sizeof(char));
    (void) state;

    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_non_null(mutable_line);
    strncpy(mutable_line, *immutable_line, line_length);

    parse_line(mutable_line, &G);

    free(mutable_line);
    mutable_line = NULL;
    librole_graph_free(&G);
}

int librole_writing_test_group_setup(void **state) {
    int result = 1;
    (void) state;

    result = unlink_test_file(file_role_test_string_new);
    if (result) {
        fail_msg("Unable to unlink %s", file_role_test_string_new);
    }

    result = unlink_test_file(file_role_test_int_new);
    if (result) {
        fail_msg("Unable to unlink %s", file_role_test_int_new);
    }

    result = unlink_test_file(file_role_test_add);
    if (result) {
        fail_msg("Unable to unlink %s", file_role_test_add);
    }

    result = unlink_test_file(file_role_test_set);
    if (result) {
        fail_msg("Unable to unlink %s", file_role_test_set);
    }

    result = unlink_test_file(file_role_test_del);
    if (result) {
        fail_msg("Unable to unlink %s", file_role_test_del);
    }

    result = unlink_test_file(file_role_test_drop);
    if (result) {
        fail_msg("Unable to unlink %s", file_role_test_drop);
    }

    return result;
}

int librole_writing_test_setup(void **state) {
    struct librole_graph *rolegraph = malloc(sizeof(struct librole_graph));
    int result = 0;

    if (!rolegraph) {
        goto librole_writing_test_setup_fail;
    }

    result = librole_graph_init(rolegraph);
    if (LIBROLE_OK != result) {
        goto librole_writing_test_setup_fail;
    }

    result = librole_reading(__LIBROLE_TEST_DATADIR "/role_file" LIBROLE_ROLE_EXTENSION, rolegraph);
    if (LIBROLE_OK != result) {
        goto librole_writing_test_setup_fail;
    }

    goto librole_writing_test_setup_success;

librole_writing_test_setup_fail:
    if (rolegraph) {
        free(rolegraph);
        rolegraph = NULL;
    }
    result = 1;
    fail_msg("Failed to setup librole_writing test");

librole_writing_test_setup_success:
    (*state) = rolegraph;

    return result;
}

int librole_writing_test_teardown(void **state) {
    struct librole_graph *rolegraph = NULL;

    rolegraph = (struct librole_graph*)(*state);

    librole_graph_free(rolegraph);
    free(rolegraph);
    rolegraph = NULL;

    return 0;
}

void test_librole_writing_to_file(void **state) {
    struct librole_graph *rolegraph = (struct librole_graph*)(*state);

    assert_int_equal(librole_writing(file_role_test_string_new, rolegraph, 0, 0, NULL), LIBROLE_OK);
    assert_file_content(file_role_test_string_new, "users:video,wheel\n");
    assert_int_equal(librole_writing(file_role_test_int_new, rolegraph, 1, 0, NULL), LIBROLE_OK);
    assert_file_content(file_role_test_int_new, "100:482,5\n");
}

void test_librole_writing_to_file_addgroup(void **state) {
    struct librole_graph *rolegraph = (struct librole_graph*)(*state);
    struct librole_ver *test_role = malloc(sizeof(struct librole_ver));
    gid_t gid = 0;

    if (!test_role) {
        fail_msg("Unable to allocate memory for 'struct librole_ver testrole'");
    }

    assert_int_equal(librole_ver_init(test_role), LIBROLE_OK);

    assert_int_equal(librole_get_gid("users", &test_role->gid), LIBROLE_OK);
    assert_int_equal(librole_get_gid("audio", &gid), LIBROLE_OK);
    assert_int_equal(librole_ver_add(test_role, gid), LIBROLE_OK);

    assert_int_equal(librole_role_add(rolegraph, *test_role), LIBROLE_OK);
    assert_int_equal(librole_writing(file_role_test_add, rolegraph, 0, 0, NULL), LIBROLE_OK);
    assert_file_content(file_role_test_add, "users:video,wheel,audio\n");

    librole_ver_free(test_role);
    free(test_role);
}

void test_librole_writing_to_file_setgroup(void **state) {
    struct librole_graph *rolegraph = (struct librole_graph*)(*state);
    struct librole_ver *test_role = malloc(sizeof(struct librole_ver));
    gid_t gid = 0;

    if (!test_role) {
        fail_msg("Unable to allocate memory for 'struct librole_ver testrole'");
    }

    assert_int_equal(librole_ver_init(test_role), LIBROLE_OK);

    assert_int_equal(librole_get_gid("users", &test_role->gid), LIBROLE_OK);
    assert_int_equal(librole_get_gid("audio", &gid), LIBROLE_OK);
    assert_int_equal(librole_ver_add(test_role, gid), LIBROLE_OK);

    assert_int_equal(librole_role_set(rolegraph, *test_role), LIBROLE_OK);
    assert_int_equal(librole_writing(file_role_test_set, rolegraph, 0, 0, NULL), LIBROLE_OK);
    assert_file_content(file_role_test_set, "users:audio\n");

    /*
     * Don't try to free librole_ver using librole_ver_free() because
     * it's linked to librole_graph after librole_role_set() call.
     *
     * librole_ver_free(test_role);
     */
    free(test_role);
}

void test_librole_writing_to_file_delgroup(void **state) {
    struct librole_graph *rolegraph = (struct librole_graph*)(*state);
    struct librole_ver *test_role = malloc(sizeof(struct librole_ver));
    gid_t gid = 0;

    if (!test_role) {
        fail_msg("Unable to allocate memory for 'struct librole_ver test_role'");
    }

    assert_int_equal(librole_ver_init(test_role), LIBROLE_OK);

    assert_int_equal(librole_get_gid("users", &test_role->gid), LIBROLE_OK);
    assert_int_equal(librole_get_gid("video", &gid), LIBROLE_OK);
    assert_int_equal(librole_ver_add(test_role, gid), LIBROLE_OK);

    assert_int_equal(librole_role_del(rolegraph, *test_role), LIBROLE_OK);
    assert_int_equal(librole_writing(file_role_test_del, rolegraph, 0, 0, NULL), LIBROLE_OK);
    assert_file_content(file_role_test_del, "users:wheel\n");

    librole_ver_free(test_role);
    free(test_role);
}

void test_librole_writing_to_file_dropgroup(void **state) {
    struct librole_graph *rolegraph = (struct librole_graph*)(*state);
    struct librole_ver *test_role = malloc(sizeof(struct librole_ver));
    gid_t gid = 0;

    if (!test_role) {
        fail_msg("Unable to allocate memory for 'struct librole_ver test_role'");
    }

    assert_int_equal(librole_ver_init(test_role), LIBROLE_OK);

    assert_int_equal(librole_get_gid("users", &test_role->gid), LIBROLE_OK);
    assert_int_equal(librole_get_gid("video", &gid), LIBROLE_OK);
    assert_int_equal(librole_ver_add(test_role, gid), LIBROLE_OK);

    assert_int_equal(librole_role_drop(rolegraph, *test_role), LIBROLE_OK);
    assert_int_equal(librole_writing(file_role_test_drop, rolegraph, 0, 0, NULL), LIBROLE_OK);
    assert_file_content(file_role_test_drop, "");

    librole_ver_free(test_role);
    free(test_role);
}
