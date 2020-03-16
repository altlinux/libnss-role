
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "role/parser.h"
#include "role/version.h"
#include "role/fileop.h"

struct option rolelst_opt[] = {
    {"help", no_argument, 0, 'h'},
    {"numeric", no_argument, 0, 'n'},
    {"version", no_argument, 0, 'v'},
    {"verbose", no_argument, 0, 'V'}
};

struct rolelst_settings {
    int numeric_flag;
    int verbose_mode;
};

static int parse_options(int argc, char **argv, struct rolelst_settings *settings)
{
    int c, opt_ind;
    while((c = getopt_long(argc, argv, "hnvV", rolelst_opt, &opt_ind)) != -1) {
        switch(c) {
            case 'h':
                fprintf(stdout, "Usage: rolelst [-hnv]\n");
                fprintf(stdout,
                    "\t-h [ --help   ]\t\tproduce help message\n");
                fprintf(stdout,
                    "\t-n [ --numeric]\t\tprint gid instead of group names\n");
                fprintf(stdout,
                    "\t-v [ --version]\t\tprint roledel version being used\n");
                fprintf(stdout,
                    "\t-V [ --verbose]\t\tprint extra information about role sources");
                fprintf(stdout, "\n");
                fflush(stdout);
                return 0;
            case 'n':
                settings->numeric_flag = 1;
                break;
            case 'V':
                settings->verbose_mode = 1;
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
    return 1;
}

int main(int argc, char **argv) {
    struct rolelst_settings settings;
    settings.numeric_flag = 0;
    settings.verbose_mode = 0;
    struct librole_graph G;
    int result;

    if (!parse_options(argc, argv, &settings))
        return 0;

    result = librole_graph_init(&G);
    if (result != LIBROLE_OK)
        goto exit;

    result = librole_reading(LIBROLE_CONFIG, &G);
    if (result != LIBROLE_OK)
        goto exit;
    if (0 != settings.verbose_mode) {
        printf("# Settings read from /etc/role:\n");
        fflush(stdout);
        result = librole_writing("/dev/stdout", &G, settings.numeric_flag);
        if (LIBROLE_OK != result) {
            goto exit;
        }
    }

    /* Don't check return code in order to retain previous utility
     * behavior */
    result = librole_get_directory_files(LIBROLE_CONFIG_DIR, &G);
    if (0 != settings.verbose_mode) {
        printf("# Resulting settings merged with /etc/role.d entries\n");
        fflush(stdout);
    }
    result = librole_writing("/dev/stdout", &G, settings.numeric_flag);
    if (LIBROLE_OK != result) {
        goto exit;
    }

exit:
    librole_print_error(result);
    librole_graph_free(&G);
    return result;
}

