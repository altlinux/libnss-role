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

#if !defined(__LIBROLE_TEST_FILEOP_RW_H)
#define __LIBROLE_TEST_FILEOP_RW_H 1

int fileop_rw_test_setup(void **state);
int fileop_rw_test_teardown(void **state);

void test_lock_existing_file(void **state);
void test_lock_stale(void **state);
void test_write_file_error(void **state);
void test_write_file_replace(void **state);

#endif
