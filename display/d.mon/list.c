#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "proto.h"

/* get list of running monitors */
void list_mon(char ***list, int *n)
{
    int i;
    const char *name;
    const char *env_prefix = "MONITOR_";
    int env_prefix_len;
    char **tokens;
    
    env_prefix_len = strlen(env_prefix);
    
    *list = NULL;
    *n    = 0;
    tokens = NULL;
    for (i = 0; (name = G__env_name(i)); i++) {
	if (strncmp(env_prefix, name, env_prefix_len) == 0) {
	    tokens = G_tokenize(name, "_");
	    if (G_number_of_tokens(tokens) != 3 ||
		strcmp(tokens[2], "ENVFILE") != 0)
		continue;
	    *list = G_realloc(*list, (*n + 1) * sizeof(char *));
	    (*list)[*n] = G_store(tokens[1]);
	    (*n)++;
	    G_free_tokens(tokens);
	    tokens = NULL;
	}
    }
    
}

/* print list of running monitors */
void print_list(FILE *fd)
{
    char **list;
    int   i, n;

    list_mon(&list, &n);
    if (n > 0)
	G_message(_("List of running monitors:"));
    else {
	G_important_message(_("No monitors running"));
	return;
    }
    
    for (i = 0; i < n; i++)
	fprintf(fd, "%s\n", list[i]);
}

/* check if monitor is running */
int check_mon(const char *name)
{
    char *env_name;
    const char *str;
    
    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_ENVFILE", name);
    str = G__getenv(env_name);
    if (!str)
	return FALSE;
    
    return TRUE;
}

/* list related commands for given monitor */
void list_cmd(const char *name, FILE *fd_out)
{
    char buf[1024];
    char *cmd_name;
    const char *cmd_value;
    FILE *fd;

    cmd_name = NULL;
    G_asprintf(&cmd_name, "MONITOR_%s_CMDFILE", name);
    cmd_value = G__getenv(cmd_name);
    if (!cmd_value)
	G_fatal_error(_("Command file not found"));
    
    fd = fopen(cmd_value, "r");
    if (!fd)
	G_fatal_error(_("Unable to read command file"));

    while (G_getl2(buf, sizeof(buf) - 1, fd) != 0) {
	fprintf(fd_out, "%s\n", buf);
    }
}
