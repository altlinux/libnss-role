/* cmocka requirements */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/* Test requirements */
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "role/parser.h"

static void test_main(void **state) {
    (void) state;
    struct librole_graph G;

    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_reading("test/role.source", &G), LIBROLE_OK);
    assert_int_equal(librole_writing("/dev/stdout", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("/dev/stdout", &G, 1), LIBROLE_OK);
    assert_int_equal(librole_writing("/dev/stdout", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("/dev/stdout", &G, 1), LIBROLE_OK);
    assert_int_equal(librole_writing("test/role.test.new", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("test/role.test.add", &G, 0), LIBROLE_OK);
    assert_int_equal(librole_writing("test/role.test.del", &G, 0), LIBROLE_OK);

    librole_graph_free(&G);
}

int main(int argc, char **argv) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_main),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

