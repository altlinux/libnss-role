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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test_config.h"
#include "test_fileop_rw.h"
#include "role/fileop_rw.h"
#include "role/lock_file.h"
#include "role/parser.h"
#include "role/glob.h"

#define GID_USERS 100
#define GID_AUDIO 50
/* not in data/group */
#define GID_MISSING 30000

#define ROLE_CONTENT "users:video,wheel\n"

static const char *role_file = __LIBROLE_TEST_DATADIR "/role.test.rw" LIBROLE_ROLE_EXTENSION;
static const char *role_file_lock = __LIBROLE_TEST_DATADIR "/role.test.rw" LIBROLE_ROLE_EXTENSION ".lock";
static const char *role_file_new = __LIBROLE_TEST_DATADIR "/role.test.rw" LIBROLE_ROLE_EXTENSION ".new";

static void write_file(const char *filename, const char *content)
{
    FILE *f = fopen(filename, "w");

    assert_non_null(f);
    assert_int_equal(fputs(content, f) < 0, 0);
    assert_int_equal(fclose(f), 0);
}

static void assert_file_content(const char *filename, const char *content)
{
    char buf[256];
    size_t len;
    FILE *f = fopen(filename, "r");

    assert_non_null(f);
    len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';
    assert_string_equal(buf, content);
}

static int file_exists(const char *filename)
{
    struct stat sb;

    return stat(filename, &sb) == 0;
}

int fileop_rw_test_setup(void **state)
{
    (void) state;
    unlink(role_file_lock);
    unlink(role_file_new);
    write_file(role_file, ROLE_CONTENT);
    return chmod(role_file, 0640);
}

int fileop_rw_test_teardown(void **state)
{
    (void) state;
    unlink(role_file_lock);
    unlink(role_file_new);
    unlink(role_file);
    return 0;
}

/* Lock is created for an existing file which stays untouched */
void test_lock_existing_file(void **state)
{
    (void) state;
    assert_int_equal(librole_lock(role_file), LIBROLE_OK);
    assert_true(file_exists(role_file_lock));
    /* Locked by a live process (this one) */
    assert_int_equal(librole_lock(role_file), LIBROLE_LOCKED);
    assert_int_equal(librole_unlock(role_file), LIBROLE_OK);
    assert_false(file_exists(role_file_lock));
    assert_file_content(role_file, ROLE_CONTENT);
}

/* Lock of a process which is gone is taken over */
void test_lock_stale(void **state)
{
    char pid_str[32];
    pid_t pid;

    (void) state;
    pid = fork();
    assert_true(pid >= 0);
    if (pid == 0)
        _exit(0);
    assert_int_equal(waitpid(pid, NULL, 0), pid);

    snprintf(pid_str, sizeof(pid_str), "%lu", (unsigned long) pid);
    write_file(role_file_lock, pid_str);

    assert_int_equal(librole_lock(role_file), LIBROLE_OK);
    assert_int_equal(librole_unlock(role_file), LIBROLE_OK);
    assert_false(file_exists(role_file_lock));
}

/* The file is not truncated if writing fails */
void test_write_file_error(void **state)
{
    struct librole_graph G;
    struct librole_ver role;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_ver_init(&role), LIBROLE_OK);
    role.gid = GID_USERS;
    assert_int_equal(librole_ver_add(&role, GID_MISSING), LIBROLE_OK);
    assert_int_equal(librole_graph_add(&G, role), LIBROLE_OK);

    assert_int_not_equal(librole_write_file(role_file, &G, 0), LIBROLE_OK);
    assert_file_content(role_file, ROLE_CONTENT);
    assert_false(file_exists(role_file_new));
    assert_false(file_exists(role_file_lock));

    librole_graph_free(&G);
}

/* The file is replaced keeping its permissions */
void test_write_file_replace(void **state)
{
    struct librole_graph G;
    struct librole_ver role;
    struct stat sb;

    (void) state;
    assert_int_equal(librole_graph_init(&G), LIBROLE_OK);
    assert_int_equal(librole_reading(role_file, &G), LIBROLE_OK);
    assert_int_equal(librole_ver_init(&role), LIBROLE_OK);
    role.gid = GID_USERS;
    assert_int_equal(librole_ver_add(&role, GID_AUDIO), LIBROLE_OK);
    assert_int_equal(librole_role_add(&G, role), LIBROLE_OK);
    librole_ver_free(&role);

    assert_int_equal(librole_write_file(role_file, &G, 0), LIBROLE_OK);
    assert_file_content(role_file, "users:video,wheel,audio\n");
    assert_int_equal(stat(role_file, &sb), 0);
    assert_int_equal(sb.st_mode & 07777, 0640);
    assert_false(file_exists(role_file_new));
    assert_false(file_exists(role_file_lock));

    librole_graph_free(&G);
}
