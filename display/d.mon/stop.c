#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "proto.h"

static int stop_wx(const char *);
static int stop(const char *);

int stop_mon(const char *name)
{
    if (!check_mon(name)) {
	G_fatal_error(_("Monitor <%s> is not running"), name);
    }
    
    if (strncmp(name, "wx", 2) == 0)
	stop_wx(name);

    return stop(name);
}

int stop(const char *name)
{
    char *mon_path, file_path[GPATH_MAX];
    struct dirent *dp;
    DIR *dirp;

    mon_path = get_path(name, TRUE);
    dirp = opendir(mon_path);

    while ((dp = readdir(dirp)) != NULL) {
        if (!*dp->d_name || dp->d_name[0] == '.')
            continue;
        sprintf(file_path, "%s/%s", mon_path, dp->d_name);
        if (unlink(file_path) == -1)
            G_warning(_("Unable to delete file <%s>"), file_path);
    }
    closedir(dirp);
    
    if (rmdir(mon_path) == -1)
        G_warning(_("Unable to delete directory <%s>"), mon_path);

    G_free(mon_path);

    G_unsetenv("MONITOR");

    return 0;
}

int stop_wx(const char *name)
{
    char *mon_path, *pid;
    char pid_file[GPATH_MAX], buf[512];
    FILE *fp;
    
    mon_path = get_path(name, FALSE);
    G_file_name(pid_file, mon_path, "pid", G_mapset());
    
    fp = fopen(pid_file, "r");
    if (!fp) {
	G_warning(_("Unable to open file <%s>"), pid_file);
        return 1;
    }
    pid = NULL;
    if (G_getl2(buf, sizeof(buf) - 1, fp) != 0)
        pid = G_store(buf);
    fclose(fp);
    
    if (!pid) {
	G_warning(_("Unable to read file <%s>"), pid_file);
        return 1;
    }
    
#ifdef __MINGW32__
    /* TODO */
#else
    if (kill((pid_t) atoi(pid), SIGTERM) != 0) {
	/* G_fatal_error(_("Unable to stop monitor <%s>"), name); */
    }
#endif
    
    return 0;
}
