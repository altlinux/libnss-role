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

#if !defined(__LIBROLE_TEST_FILES_H)
#define __LIBROLE_TEST_FILES_H 1

int files_test_setup(void **state);
int files_test_teardown(void **state);

void test_reading_missing_file(void **state);
void test_nss_initgroups_missing_file(void **state);
void test_find_role_file(void **state);

#endif
