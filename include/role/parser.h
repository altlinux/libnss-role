#ifndef LIBROLE_PARSER_H
#define LIBROLE_PARSER_H

#include <grp.h>
#include <sys/types.h>
#include <role/glob.h>

struct librole_ver {
	gid_t gid;
	gid_t *list;
	int size;
	int capacity;
};

typedef struct librole_ver librole_group_collector;

struct librole_graph {
	struct librole_ver *gr;
	int *used;
	int size;
	int capacity;
};

int librole_graph_add(struct librole_graph *, struct librole_ver);

int librole_ver_add(struct librole_ver *, gid_t);

int librole_graph_init(struct librole_graph *);

int librole_ver_init(struct librole_ver *);

void librole_graph_free(struct librole_graph *);
void librole_ver_free(struct librole_ver *);

int librole_realloc_groups(long int **, gid_t ***, long int);

int librole_reading(const char *, struct librole_graph *);

int librole_writing(const char *, struct librole_graph *, int numeric_flag);
int librole_write(const char* pam_role, struct librole_graph *G);

int librole_dfs(struct librole_graph *, gid_t, librole_group_collector *);

int librole_find_gid(struct librole_graph *v, gid_t g, int *idx);
int librole_ver_find_gid(struct librole_ver *v, gid_t g, int *idx);

int librole_role_add(struct librole_graph *G, struct librole_ver new_role);
int librole_role_set(struct librole_graph *G, struct librole_ver new_role);

int librole_role_del(struct librole_graph *G, struct librole_ver del_role);
int librole_role_drop(struct librole_graph *G, struct librole_ver del_role);

/* get gid by group name */
int librole_get_gid(const char *gr_name, gid_t *ans);

/* get user name by uid */
int librole_get_user_name(uid_t uid, char *user_name, size_t user_name_size);

/* get group name by uid */
int librole_get_group_name(gid_t g, char *ans, size_t ans_size);

void librole_print_error(int result);

/* internal */
int librole_realloc_buffer(void **buffer, size_t *size);
int librole_en_vector(void **buffer, size_t *capacity, size_t used, size_t elsize);

int librole_create_ver_from_args(int argc, char **argv, int optind, struct librole_ver *new_role, int skip_flag);

void drop_quotes(char **str);
int parse_line(char *line, struct librole_graph *G);

#endif
