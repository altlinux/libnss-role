/*
 * Copyright (c) 2008-2020 Etersoft
 * Copyright (c) 2020 BaseALT
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "role/parser.h"
#include "role/version.h"
#include "role/fileop.h"
#include "role/paths.h"
#include "role/fileop_rw.h"


struct option rolelst_opt[] = {
    {"help", no_argument, 0, 'h'},
    {"numeric", no_argument, 0, 'n'},
    {"version", no_argument, 0, 'v'},
    {"verbose", no_argument, 0, 'V'},
    {"system", optional_argument, 0, 'S'},
    {"file", required_argument, 0, 'f'},
    {0, 0, 0, 0}
};

struct rolelst_settings {
    int numeric_flag;
    int verbose_mode;
    int system_role_mode;
    int roles_list_mode;
    const char* roled_filename;
    char* system_role;
};

static char** roles_list = NULL;
int roles_list_count = 0;

static int parse_options(int argc, char **argv, struct rolelst_settings *settings)
{
    int c, opt_ind;
    while((c = getopt_long(argc, argv, "hnvVS::f:", rolelst_opt, &opt_ind)) != -1) {
        switch(c) {
            case 'h':
                fprintf(stdout, "Usage: rolelst [-hnvV] [-f FILE.role | -S [ROLE]] [ROLE ...]\n");
                fprintf(stdout,
                    "\t-h [ --help   ]\t\tproduce help message\n");
                fprintf(stdout,
                    "\t-n [ --numeric]\t\tprint gid instead of group names\n");
                fprintf(stdout,
                    "\t-v [ --version]\t\tprint rolelst version being used\n");
                fprintf(stdout,
                    "\t-V [ --verbose]\t\tprint extra information about role sources\n");
                fprintf(stdout,
                    "\t-f [ --file=file.role ]\tshow role in additional file /etc/role.d/file.role\n");
                fprintf(stdout,
                    "\t-S [ --system[=ROLE] ]\tshow system ROLE in additional file /etc/role.d/ROLE.role");
                fprintf(stdout, "\n");
                fflush(stdout);
                return 0;
            case 'n':
                settings->numeric_flag = 1;
                break;
            case 'V':
                settings->verbose_mode = 1;
                break;
            case 'f':
                if (settings->system_role_mode) {
                    fprintf(stderr, "rolelst: show role in additional file option excludes show system role option\n");
                    return 0;
                }
                settings->roled_filename = optarg;
                break;
            case 'S':
                if (settings->roled_filename) {
                    fprintf(stderr, "rolelst: show system role option excludes show role in additional file option\n");
                    return 0;
                }
                settings->system_role_mode = 1;
                if (optarg == NULL && argv[optind] != NULL && argv[optind][0] != '-') {
                    settings->system_role = argv[optind];
                    ++optind;
                } else {
                    settings->system_role = optarg;
                }
                break;
            case 'v':
                printf("rolelst is the utility for libnss_role version %s\n",
                       LIBNSS_ROLE_VERSION);
                fflush(stdout);
                return 0;
            case '?':
                return 0;
            default:
                return 0;
        }
    }

    if (!settings->system_role_mode && argc > optind && argv[optind] != NULL) {
        roles_list = argv + optind;
        roles_list_count = argc - optind;
        settings->roles_list_mode = 1;
    }
    return 1;
}

static librole_roles_filter roles_filter = NULL;
static char* system_roles[LIBROLE_MAX_SYSTEM_ROLES];

int system_roles_filter(const char *rolename) {
    int i = 0;
    char *system_role;

    while (i < LIBROLE_MAX_SYSTEM_ROLES) {
        system_role = system_roles[i++];

        if (system_role == NULL)
            break;
        if (strcmp(system_role, rolename) == 0)
            return 1;
    }

    return 0;
}

int roles_list_filter(const char *rolename) {
    int i = 0;
    char *role;

    while (i < roles_list_count) {
        role = roles_list[i++];

        if (role == NULL)
            break;
        if (strcmp(role, rolename) == 0)
            return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    struct rolelst_settings settings;
    struct librole_graph G = {0};
    int result = LIBROLE_OK;
    int source_unavail = 0;
    memset(&settings, 0, sizeof(settings));
    memset(system_roles, 0, sizeof(system_roles));

    if (!parse_options(argc, argv, &settings))
        return 0;

    result = librole_graph_init(&G);
    if (result != LIBROLE_OK)
        goto exit;

    if (settings.roled_filename) {
        result = librole_validate_filename_from_dir(settings.roled_filename);
        if (result != LIBROLE_OK)
            goto exit;

        result = librole_read_file_from_dir(librole_config_dir(), settings.roled_filename, &G);
        if (result == LIBROLE_SOURCE_UNAVAIL)
            source_unavail = 1;
        else if (result != LIBROLE_OK)
            goto exit;
    } else {
        if (settings.system_role_mode) {
            if (settings.system_role) {
                system_roles[0] = settings.system_role;
                system_roles[1] = NULL;
            } else {
                result = librole_get_system_roles(librole_config_dir(), system_roles);
                if (result != LIBROLE_OK)
                    goto exit;
            }
            roles_filter = system_roles_filter;
        }

        result = librole_reading(librole_config_file(), &G);
        /* Show what can be resolved, but report incomplete output */
        if (result == LIBROLE_SOURCE_UNAVAIL)
            source_unavail = 1;
        else if (result != LIBROLE_OK)
            goto exit;
    }

    if (settings.roles_list_mode) {
        roles_filter = roles_list_filter;
    }

    if ((settings.verbose_mode || settings.roled_filename) && !settings.system_role_mode) {
        if (settings.verbose_mode && settings.roled_filename) {
            printf("# Settings read from /etc/role.d/%s:\n", settings.roled_filename);
        } else if (settings.verbose_mode) {
            printf("# Settings read from /etc/role:\n");
        }

        fflush(stdout);
        result = librole_writing("/dev/stdout", &G, settings.numeric_flag, 0, roles_filter);
        if (LIBROLE_OK != result || settings.roled_filename) {
            goto exit;
        }
    }


    result = librole_get_directory_files(librole_config_dir(), &G);
    if (result == LIBROLE_SOURCE_UNAVAIL)
        source_unavail = 1;
    else if (result != LIBROLE_OK)
        goto exit;

    if (0 != settings.verbose_mode) {
        if (settings.system_role_mode) {
            if (settings.system_role == NULL) {
                printf("# Resulting settings for system roles\n");
            } else {
                printf("# Settings read from system role '%s':\n", settings.system_role);
            }
        } else {
            printf("# Resulting settings merged with /etc/role.d entries\n");
        }
        fflush(stdout);
    }
    result = librole_writing("/dev/stdout", &G, settings.numeric_flag, 1, roles_filter);
    if (LIBROLE_OK != result) {
        goto exit;
    }

exit:
    if (result == LIBROLE_OK && source_unavail)
        result = LIBROLE_SOURCE_UNAVAIL;
    librole_print_error(result);
    librole_graph_free(&G);
    return result;
}

