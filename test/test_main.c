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

#include "role/parser.h"

static void test_drop_quotes(void **state) {
    (void) state;

    /* Create mutable memory area for test string */
    char *immutable_line[] = { "\"role_name\"" };
    size_t line_length = strlen(*immutable_line);
    char *mutable_line = calloc(line_length + 1, sizeof(char));
    assert_non_null(mutable_line);
    /* drop_quotes increments the pointer making it impossible to free
     * the memory so it is needed to store the original pointer to
     * be able to free the memory block allocated */
    char *orig_ptr = mutable_line;
    strncpy(mutable_line, *immutable_line, line_length);
    strcpy(mutable_line, *immutable_line);

    assert_string_equal(mutable_line, "\"role_name\"");
    drop_quotes(&mutable_line);
    assert_string_equal(mutable_line, "role_name");

    free(orig_ptr);
    mutable_line = NULL;
    orig_ptr = NULL;
}

static void test_parse_line(void **state) {
    struct librole_graph G;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    char *immutable_line[] = { "users:\"tftp\",named" };
    size_t line_length = strlen(*immutable_line);
    char *mutable_line = calloc(line_length + 1, sizeof(char));
    assert_non_null(mutable_line);
    strncpy(mutable_line, *immutable_line, line_length);

    parse_line(mutable_line, &G);

    free(mutable_line);
    mutable_line = NULL;
}

static void test_main(void **state) {
    (void) state;
    struct librole_graph G;

    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_reading("test/role.source", &G), LIBROLE_OK);
    assert_int_equal(librole_writing("/dev/stdout", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("/dev/stdout", &G, 1), LIBROLE_OK);
    assert_int_equal(librole_writing("test/role.test.new", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("test/role.test.add", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("test/role.test.del", &G, 0), LIBROLE_OK);

    librole_graph_free(&G);
}

int main(int argc, char **argv) {
    const struct CMUnitTest tests[] = {
          cmocka_unit_test(test_drop_quotes)
        , cmocka_unit_test(test_parse_line)
        /*cmocka_unit_test(test_main)*/
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

