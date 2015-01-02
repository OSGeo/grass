#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include <grass/display.h>
#include <grass/glocale.h>

#include "proto.h"

static void start(const char *, const char *, int);
static void start_wx(const char *, const char *, const char *,
		     const char *, int, int);
static void error_handler(void *);

/* start file-based monitor */
void start(const char *name, const char *output, int update)
{
    char *env_name, output_path[GPATH_MAX];
    const char *output_name;
    
    /* stop monitor on failure */
    G_add_error_handler(error_handler, (char *)name);
    
    if (!output) {
        D_open_driver();
        
        output_name = D_get_file();
        if (!output_name) 
            return;
        if (!update && access(output_name, F_OK) == 0) {
            if (G_get_overwrite()) {
                G_warning(_("File '%s' already exists and will be overwritten"), output_name);
                D_setup_unity(0);
                D_erase("white");
            }
            else {
                D_close_driver();
                G_fatal_error(_("option <%s>: <%s> exists."),
                              "output", output_name);
            }
        }
        D_close_driver(); /* must be called after check because this
                           * function produces default map file */
    }
    else {
        output_name = output;
    }

        
    if (!strchr(output_name, HOST_DIRSEP)) { /* relative path */
        char *ptr;
        
        if (!getcwd(output_path, GPATH_MAX))
            G_fatal_error(_("Unable to get current working directory"));
        ptr = output_path + strlen(output_path) - 1;
        if (*(ptr++) != HOST_DIRSEP) {
            *(ptr++) = HOST_DIRSEP;
            *(ptr) = '\0';
        }
        strcat(output_path, output_name);
        G_message(_("Output file: %s"), output_path);
    }
    else {
        strcpy(output_path, output_name); /* already full path */
    }

    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_MAPFILE", G_store_upper(name));
    G_setenv(env_name, output_path);
}

/* start wxGUI display monitor */
void start_wx(const char *name, const char *tempfile,
	      const char *env_value, const char *cmd_value,
	      int width, int height)
{
    char progname[GPATH_MAX];
    char *env_name, *map_value, str_width[1024], str_height[1024];

    env_name = NULL;
    G_asprintf(&env_name, "MONITOR_%s_MAPFILE", G_store_upper(name));
    G_asprintf(&map_value, "%s.ppm", tempfile);
    G_setenv(env_name, map_value);
    /* close(creat(map_value, 0666)); */
    
    G_debug(3, "       mapfile = %s", map_value);

    sprintf(progname, "%s/gui/wxpython/mapdisp/main.py", G_gisbase());
    if (width > 0)
        sprintf(str_width, "%d", width);
    else
        str_width[0] = '\0';
    if (height > 0)
        sprintf(str_height, "%d", height);
    else
        str_height[0] = '\0';

    G_spawn_ex(getenv("GRASS_PYTHON"), progname, progname,
	       name, map_value, cmd_value, env_value,
               str_width, str_height, SF_BACKGROUND, NULL);
}

int start_mon(const char *name, const char *output, int select,
	      int width, int height, const char *bgcolor,
	      int truecolor, int update)
{
    char *u_name;
    char *env_name, *env_value, *cmd_value;
    char *tempfile, buf[1024];
    int env_fd;

    if (check_mon(name)) {
        const char *curr_mon;

        curr_mon = G_getenv_nofatal("MONITOR");
	if (select && (!curr_mon || strcmp(curr_mon, name) != 0))
	    G_setenv("MONITOR", name);

        G_fatal_error(_("Monitor <%s> already running"), name);
    }

    tempfile = G_tempfile();

    u_name = G_store_upper(name);

    env_name = env_value = NULL;
    G_asprintf(&env_name, "MONITOR_%s_ENVFILE", u_name);
    G_asprintf(&env_value, "%s.env", tempfile);
    G_setenv(env_name, env_value);
    env_fd = creat(env_value, 0666);
    if (env_fd < 0)
	G_fatal_error(_("Unable to create file '%s'"), env_value);

    sprintf(buf, "GRASS_RENDER_FILE_READ=TRUE\n");
    write(env_fd, buf, strlen(buf));
    if (width) {
	sprintf(buf, "GRASS_RENDER_WIDTH=%d\n", width);
	write(env_fd, buf, strlen(buf));
    }
    if (height) {
	sprintf(buf, "GRASS_RENDER_HEIGHT=%d\n", height);
	write(env_fd, buf, strlen(buf));
    }
    if (bgcolor) {
	if (strcmp(bgcolor, "none") == 0)
	    sprintf(buf, "GRASS_RENDER_TRANSPARENT=TRUE\n");
	else
	    sprintf(buf, "GRASS_RENDER_BACKGROUNDCOLOR=%s\n", bgcolor);
	write(env_fd, buf, strlen(buf));
    }
    if (truecolor) {
	sprintf(buf, "GRASS_RENDER_TRUECOLOR=TRUE\n");
	write(env_fd, buf, strlen(buf));
    }
    close(env_fd);

    cmd_value = NULL;
    G_asprintf(&env_name, "MONITOR_%s_CMDFILE", u_name);
    G_asprintf(&cmd_value, "%s.cmd", tempfile);
    G_setenv(env_name, cmd_value);
    close(creat(cmd_value, 0666));

    G_verbose_message(_("Starting monitor <%s> with env file '%s'"), name, env_value);
    if (G_verbose() > G_verbose_std()) {
        FILE *fd;
        
        fd = fopen(env_value, "r");
        while (G_getl2(buf, sizeof(buf) - 1, fd) != 0) {
            fprintf(stderr, " %s\n", buf);
        }
        fclose(fd);
    }

    G_debug(1, "start: name=%s ", name);
    G_debug(3, "       envfile = %s", env_value);
    G_debug(3, "       cmdfile = %s", cmd_value);
    
    if (select)
	G_setenv("MONITOR", name);
    
    if (strncmp(name, "wx", 2) == 0) 
	start_wx(name, tempfile, env_value, cmd_value, 
		 width, height);
    else
      start(name, output, update);
    
    return 0;
}

void error_handler(void *p)
{
    const char *name = (const char *) p;
    stop_mon(name);
}
