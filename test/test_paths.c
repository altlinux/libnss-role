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
/* cmocka requirements */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "test_config.h"
#include "test_paths.h"
#include "role/paths.h"

static const char *testdir = __LIBROLE_TEST_DATADIR;

const char *librole_config_dir(void) {
	return mock_ptr_type(const char *);
}

void test_librole_config_vars(void **state) {
    (void) state;
    will_return(librole_config_dir, testdir);
    assert_string_equal(librole_config_dir(), testdir);
}

