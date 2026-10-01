/*
 * Copyright (c) 2021 BaseALT Ltd. <org@basealt.ru>
 * Copyright (c) 2021 Igor Chudov <nir@nir.org.ru>
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

#define UNIT_TESTING 1

/* cmocka requirements */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#include "test_config.h"
#include "test_parser.h"
#include "test_paths.h"
#include "test_unavail.h"
#include "test_memory.h"
#include "test_fileop_rw.h"
#include "test_roles.h"
#include "test_files.h"


int main(void) {
    int result = 0;

    const struct CMUnitTest parser_tests[] = {
          cmocka_unit_test(test_drop_quotes)
        , cmocka_unit_test(test_parse_line)
    };

    const struct CMUnitTest librole_writing_tests[] = {
          cmocka_unit_test_setup_teardown(
              test_librole_writing_to_file
            , librole_writing_test_setup
            , librole_writing_test_teardown)
        , cmocka_unit_test_setup_teardown(
              test_librole_writing_to_file_addgroup
            , librole_writing_test_setup
            , librole_writing_test_teardown)
        , cmocka_unit_test_setup_teardown(
              test_librole_writing_to_file_setgroup
            , librole_writing_test_setup
            , librole_writing_test_teardown)
        , cmocka_unit_test_setup_teardown(
              test_librole_writing_to_file_delgroup
            , librole_writing_test_setup
            , librole_writing_test_teardown)
        , cmocka_unit_test_setup_teardown(
              test_librole_writing_to_file_dropgroup
            , librole_writing_test_setup
            , librole_writing_test_teardown)
    };

    const struct CMUnitTest unavail_tests[] = {
          cmocka_unit_test(test_parse_line_existing_role_error)
        , cmocka_unit_test(test_parse_line_unavail_member)
        , cmocka_unit_test(test_parse_line_unavail_role)
        , cmocka_unit_test(test_reading_unavail)
        , cmocka_unit_test(test_reading_last_line_without_eol)
        , cmocka_unit_test(test_nss_initgroups_unavail)
        , cmocka_unit_test(test_reading_directory_unavail)
    };

    const struct CMUnitTest memory_tests[] = {
          cmocka_unit_test(test_get_group_name_buffer)
        , cmocka_unit_test(test_role_drop_then_add)
        , cmocka_unit_test(test_graph_grow)
        , cmocka_unit_test(test_validate_filename)
    };

    const struct CMUnitTest fileop_rw_tests[] = {
          cmocka_unit_test_setup_teardown(test_lock_existing_file
            , fileop_rw_test_setup, fileop_rw_test_teardown)
        , cmocka_unit_test_setup_teardown(test_lock_stale
            , fileop_rw_test_setup, fileop_rw_test_teardown)
        , cmocka_unit_test_setup_teardown(test_write_file_error
            , fileop_rw_test_setup, fileop_rw_test_teardown)
        , cmocka_unit_test_setup_teardown(test_write_file_replace
            , fileop_rw_test_setup, fileop_rw_test_teardown)
    };

    const struct CMUnitTest roles_tests[] = {
          cmocka_unit_test(test_parse_line_formats)
        , cmocka_unit_test(test_dfs_nested_and_cycle)
        , cmocka_unit_test_setup_teardown(test_reading_role_dir
            , roles_test_setup, roles_test_teardown)
        , cmocka_unit_test_setup_teardown(test_nss_initgroups
            , roles_test_setup, roles_test_teardown)
        , cmocka_unit_test_setup_teardown(test_nss_initgroups_existing_groups
            , roles_test_setup, roles_test_teardown)
    };

    const struct CMUnitTest files_tests[] = {
          cmocka_unit_test_setup_teardown(test_reading_missing_file
            , files_test_setup, files_test_teardown)
        , cmocka_unit_test_setup_teardown(test_nss_initgroups_missing_file
            , files_test_setup, files_test_teardown)
        , cmocka_unit_test_setup_teardown(test_find_role_file
            , files_test_setup, files_test_teardown)
    };

    const struct CMUnitTest paths_tests[] = {
        cmocka_unit_test(test_librole_config_vars)
    };

    result = cmocka_run_group_tests_name("parser_tests", parser_tests, NULL, NULL);
    result += cmocka_run_group_tests_name(
          "librole_writing_tests"
        , librole_writing_tests
        , librole_writing_test_group_setup
        , NULL);
    result += cmocka_run_group_tests_name("unavail_tests", unavail_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("memory_tests", memory_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("fileop_rw_tests", fileop_rw_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("roles_tests", roles_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("files_tests", files_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("paths_tests", paths_tests, NULL, NULL);

    return result;
}

