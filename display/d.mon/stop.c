#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "proto.h"

static void clean_env(const char *);
static int stop_wx(const char *);
static int stop(const char *);

int stop_mon(const char *name)
{
    if (!check_mon(name)) {
	clean_env(name);
	G_fatal_error(_("Monitor <%s> is not running"), name);
    }
    
    if (strncmp(name, "wx", 2) == 0)
	return stop_wx(name);

    return stop(name);
}

int stop(const char *name)
{
    char *env_name;
    const char *env_file;
    
    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_ENVFILE", name);
    
    env_file = G__getenv(env_name);
    if (!env_file)
	G_warning(_("Env file not found"));
    
    clean_env(name);

    return 0;
}

int stop_wx(const char *name)
{
    char *env_name;
    const char *pid;
    
    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_PID", name);
    
    pid = G__getenv(env_name);
    if (!pid) {
	clean_env(name);
	G_fatal_error(_("PID file not found"));
    }
    
    if (kill((pid_t) atoi(pid), SIGTERM) != 0) {
	/* G_fatal_error(_("Unable to stop monitor <%s>"), name); */
    }
    
    clean_env(name);

    return 0;
}

void clean_env(const char *name)
{
    int i;
    const char *env_prefix = "MONITOR_";
    const char *env;
    int env_prefix_len;
    char **tokens;
    
    env_prefix_len = strlen(env_prefix);
    
    tokens = NULL;
    for (i = 0; (env = G__env_name(i)); i++) {
	if (strncmp(env_prefix, env, env_prefix_len) != 0)
	    continue;
	
	tokens = G_tokenize(env, "_");
	if (G_number_of_tokens(tokens) != 3 ||
	    strcmp(tokens[1], name) != 0)
	    continue;
	G_unsetenv(env);
	i--; /* env has been removed for the list */
	G_free_tokens(tokens);
	tokens = NULL;
    }

    G_unsetenv("MONITOR");
}
