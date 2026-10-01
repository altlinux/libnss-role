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

#if !defined(__LIBROLE_TEST_UNAVAIL_H)
#define __LIBROLE_TEST_UNAVAIL_H 1

void test_parse_line_existing_role_error(void **state);
void test_parse_line_unavail_member(void **state);
void test_parse_line_unavail_role(void **state);
void test_reading_unavail(void **state);
void test_reading_last_line_without_eol(void **state);
void test_nss_initgroups_unavail(void **state);

void test_reading_directory_unavail(void **state);

#endif
