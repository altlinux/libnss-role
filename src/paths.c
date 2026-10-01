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
#include "role/paths.h"

static const char *role_config_file = "/etc/role";
static const char *role_config_dir = "/etc/role.d";

const char *librole_config_file(void) {
	return role_config_file;
}

const char *librole_config_dir(void) {
	return role_config_dir;
}

