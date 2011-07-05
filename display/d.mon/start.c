#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#include "proto.h"

static void start(const char *, const char *);
static void start_wx(const char *);

void start(const char *name, const char *output)
{
    char *tempfile;
    char *env_name, *env_value;
    
    tempfile = G_tempfile();

    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_ENVFILE", name);
    G_asprintf(&env_value, "%s.env", tempfile);
    G_setenv(env_name, env_value);
    close(creat(env_value, 0666));

    G_asprintf(&env_name, "MONITOR_%s_MAPFILE", name);
    G_setenv(env_name, output);
}

void start_wx(const char *name)
{
    char progname[GPATH_MAX];
    char *tempfile;
    char *env_name, *map_value, *cmd_value, *env_value;
   
    tempfile = G_tempfile();

    G_asprintf(&env_name, "MONITOR_%s_ENVFILE", name);
    G_asprintf(&env_value, "%s.env", tempfile);
    G_setenv(env_name, env_value);
    close(creat(env_value, 0666));
    
    G_asprintf(&env_name, "MONITOR_%s_CMDFILE", name);
    G_asprintf(&cmd_value, "%s.cmd", tempfile);
    G_setenv(env_name, cmd_value);
    close(creat(cmd_value, 0666));

    G_asprintf(&env_name, "MONITOR_%s_MAPFILE", name);
    G_asprintf(&map_value, "%s.ppm", tempfile);
    G_setenv(env_name, map_value);
    /* close(creat(map_value, 0666)); */
    
    G_debug(1, "start: name=%s ", name);
    G_debug(1, "       cmdfile = %s", cmd_value);
    G_debug(1, "       mapfile = %s", map_value);
    G_debug(1, "       envfile = %s", env_value);

    sprintf(progname, "%s/etc/gui/wxpython/gui_modules/mapdisp.py", G_gisbase());
    G_spawn_ex(getenv("GRASS_PYTHON"), progname, progname,
	       name, map_value, cmd_value, env_value, SF_BACKGROUND, NULL);
}

int start_mon(const char *name, const char *output, int select)
{
    const char *curr_mon;

    curr_mon = G__getenv("MONITOR");
    if (curr_mon && strcmp(curr_mon, name) == 0 && check_mon(curr_mon))
	G_fatal_error(_("Monitor <%s> already running"), name);
    
    if (select)
	G_setenv("MONITOR", name);
    
    if (strncmp(name, "wx", 2) == 0) /* use G_strncasecmp() instead */
	start_wx(name);
    else
	start(name, output);
    
    return 0;
}
