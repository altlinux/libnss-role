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
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include "role/parser.h"
#include "role/version.h"
#include "role/paths.h"
#include "role/fileop.h"
#include "role/fileop_rw.h"

struct option rolelst_opt[] = {
    {"help", no_argument, 0, 'h'},
    {"remove", no_argument, 0, 'r'},
    {"skip-missing-groups", no_argument, 0, 'm'},
    {"version", no_argument, 0, 'v'},
    {"system", no_argument, 0, 'S'},
    {"file", required_argument, 0, 'f'},
    {0, 0, 0, 0}
};

static void print_help(void)
{
    fprintf(stdout, "Usage: roledel [-hrmv] [-f FILE.role | -S] ROLE [PRIVS ...]\n");
    fprintf(stdout,
        "\t-h [ --help ]\t\tproduce help message\n");
    fprintf(stdout,
        "\t-r [ --remove ]\t\tremove all privileges from role\n");
    fprintf(stdout,
        "\t-m [ --skip-missing-groups ]\tskip missed privileges\n");
    fprintf(stdout,
        "\t-v [ --version]\t\tprint roledel version being used\n");
    fprintf(stdout,
        "\t-f [ --file=file.role ]\tchange role in additional file /etc/role.d/file.role\n");
    fprintf(stdout,
        "\t-S [ --system ]\t\tchange system ROLE in additional file /etc/role.d/ROLE.role");
    fprintf(stdout, "\n");
}

static int parse_options(int argc, char **argv, int *remove_flag, int *skip_flag, int *roled_flag, char **roled_file, int *system_role_flag)
{
    int c, opt_ind;
    *remove_flag = 0;
    *skip_flag = 0;
    *roled_flag = 0;
    *system_role_flag = 0;
    while((c = getopt_long(argc, argv, "hrmvSf:", rolelst_opt, &opt_ind)) != -1) {
        switch(c) {
            case 'h':
                print_help();
                return 0;
            case 'm':
                *skip_flag = 1;
                break;
            case 'r':
                *remove_flag = 1;
                break;
            case 'f':
                *roled_flag = 1;
                *roled_file = strdup(optarg);
                break;
            case 'S':
                *system_role_flag = 1;
                break;
            case 'v':
                printf("roledel is the utility for libnss_role version %s\n",
                       LIBNSS_ROLE_VERSION);
                return 0;
            case '?':
                return 0;
            default:
                return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    struct librole_graph G;
    int result, remove_flag, skip_flag, roled_flag, system_role_flag;
    struct librole_ver del_role;
    char *filename = NULL;

    if (!parse_options(argc, argv, &remove_flag, &skip_flag, &roled_flag, &filename, &system_role_flag))
        return 0;

    if (optind >= argc) {
        print_help();
        return 1;
    }

    if (roled_flag && system_role_flag) {
        print_help();
        return 1;
    }

    result = librole_graph_init(&G);
    if (result != LIBROLE_OK)
        goto exit;

    if (roled_flag) {
        result = librole_validate_filename_from_dir(filename);
        if (result != LIBROLE_OK)
            goto exit;

        /* A missing file is fine (a new one will be written), but don't
         * rewrite the file if some groups in it can't be resolved now */
        result = librole_read_file_from_dir(librole_config_dir(), filename, &G);
        if (result == LIBROLE_SOURCE_UNAVAIL)
            goto exit;
    } else if (system_role_flag) {
        size_t filename_sz = strlen(argv[optind]) + strlen(LIBROLE_ROLE_EXTENSION) + 1;

        filename = malloc(filename_sz);
        if (filename == NULL)
            goto exit;

        snprintf(filename, filename_sz, "%s" LIBROLE_ROLE_EXTENSION, argv[optind]);

        result = librole_validate_filename_from_dir(filename);
        if (result != LIBROLE_OK)
            goto exit;

        /* A missing file is fine (a new one will be written), but don't
         * rewrite the file if some groups in it can't be resolved now */
        result = librole_read_file_from_dir(librole_config_dir(), filename, &G);
        if (result == LIBROLE_SOURCE_UNAVAIL)
            goto exit;
    } else {
        result = librole_reading(librole_config_file(), &G);
        if (result != LIBROLE_OK)
            goto exit;
    }

    result = librole_create_ver_from_args(argc, argv, optind, &del_role, skip_flag);
    if (result != LIBROLE_OK)
        goto exit;

    if (remove_flag)
        result = librole_role_drop(&G, del_role);
    else
        result = librole_role_del(&G, del_role);

    /* The role may be in another file in /etc/role.d */
    if (result == LIBROLE_NO_SUCH_GROUP) {
        char *role_file = NULL;

        if (librole_find_role_file(librole_config_dir(), del_role.gid, &role_file) == LIBROLE_OK) {
            fprintf(stderr, "Role %s is defined in %s/%s, use: roledel -f %s ...\n",
                    argv[optind], librole_config_dir(), role_file, role_file);
            free(role_file);
        } else {
            fprintf(stderr, "Role %s is not defined\n", argv[optind]);
        }
    }

    if (result == LIBROLE_OK) {
        if (roled_flag) {
            result = librole_write_dir(filename, "roledel", &G, 0);
        } else if (system_role_flag) {
            result = librole_write_dir(filename, "roledel", &G, 1);
        } else {
            result = librole_write("roledel", &G, 0);
        }
    }

exit:
    if (filename != NULL)
        free(filename);
    librole_print_error(result);
    librole_graph_free(&G);
    return result;
}
