#if !defined(LIBROLE_FILEOP_RW_H)
#define LIBROLE_FILEOP_RW_H 1

#include <role/parser.h>

typedef int (*librole_roles_filter)(const char *rolename);

int librole_writing(const char *, struct librole_graph *, int numeric_flag, int empty_flag, librole_roles_filter filter);
int librole_write_file(const char *file, struct librole_graph *G, int empty_flag);
int librole_write(const char* pam_role, struct librole_graph *G, int empty_flag);
int librole_write_dir(const char* filename, const char* pam_role, struct librole_graph *G, int empty_flag);

#endif

