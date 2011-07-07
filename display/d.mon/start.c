#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#include "proto.h"

static void start(const char *, const char *);
static void start_wx(const char *, const char *, const char *,
		     const char *, const char *);

/* start file-based monitor */
void start(const char *name, const char *output)
{
    char *env_name;

    if (!output)
	return;
    
    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_MAPFILE", name);
    G_setenv(env_name, output);
}

/* start wxGUI display monitor */
void start_wx(const char *name, const char *tempfile, const char *env_value,
	      const char *width, const char *height)
{
    char progname[GPATH_MAX];
    char *env_name, *map_value, *cmd_value;

    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_CMDFILE", name);
    G_asprintf(&cmd_value, "%s.cmd", tempfile);
    G_setenv(env_name, cmd_value);
    close(creat(cmd_value, 0666));

    G_asprintf(&env_name, "MONITOR_%s_MAPFILE", name);
    G_asprintf(&map_value, "%s.ppm", tempfile);
    G_setenv(env_name, map_value);
    /* close(creat(map_value, 0666)); */
    
    G_debug(3, "       cmdfile = %s", cmd_value);
    G_debug(3, "       mapfile = %s", map_value);

    sprintf(progname, "%s/etc/gui/wxpython/gui_modules/mapdisp.py", G_gisbase());
    G_spawn_ex(getenv("GRASS_PYTHON"), progname, progname,
	       name, map_value, cmd_value, env_value, width ? width : "", height ? height : "", SF_BACKGROUND, NULL);
}

int start_mon(const char *name, const char *output, int select,
	      const char *width, const char *height, const char *bgcolor)
{
    const char *curr_mon;
    char *env_name, *env_value;
    char *tempfile, buf[1024];
    int env_fd;
    
    curr_mon = G__getenv("MONITOR");
    if (curr_mon && strcmp(curr_mon, name) == 0 && check_mon(curr_mon))
	G_fatal_error(_("Monitor <%s> already running"), name);
    
    tempfile = G_tempfile();

    env_name = env_value = NULL;
    G_asprintf(&env_name, "MONITOR_%s_ENVFILE", name);
    G_asprintf(&env_value, "%s.env", tempfile);
    G_setenv(env_name, env_value);
    env_fd = creat(env_value, 0666);
    if (env_fd < 0)
	G_fatal_error(_("Unable to create file '%s'"), env_value);
    if (width) {
	sprintf(buf, "GRASS_WIDTH=%s\n", width);
	write(env_fd, buf, strlen(buf));
    }
    if (height) {
	sprintf(buf, "GRASS_HEIGHT=%s\n", height);
	write(env_fd, buf, strlen(buf));
    }
    if (bgcolor) {
	if (strcmp(bgcolor, "none") == 0)
	    sprintf(buf, "GRASS_TRANSPARENT=TRUE\n");
	else
	    sprintf(buf, "GRASS_BACKGROUNDCOLOR=%s\n", bgcolor);
	write(env_fd, buf, strlen(buf));
    }
    close(env_fd);

    G_verbose_message(_("Staring monitor <%s> with env file '%s'"), name, env_value);
    
    G_debug(1, "start: name=%s ", name);
    G_debug(3, "       envfile = %s", env_value);

    if (select)
	G_setenv("MONITOR", name);
    
    if (strncmp(name, "wx", 2) == 0) /* use G_strncasecmp() instead */
	start_wx(name, tempfile, env_value, width, height);
    else
	start(name, output);
    
    return 0;
}
