#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "proto.h"

/* get monitor path */
char *get_path(const char *name, int fpath)
{
    char tmpdir[GPATH_MAX];
    
    G_temp_element(tmpdir);
    strcat(tmpdir, "/");
    strcat(tmpdir, "MONITORS");
    if (name) {
        strcat(tmpdir, "/");
        strcat(tmpdir, name);
    }

    if (fpath) {
        char ret[GPATH_MAX];
        
        G_file_name(ret, tmpdir, NULL, G_mapset());
        return G_store(ret);
    }
    
    return G_store(tmpdir);
}

/* get list of running monitors */
void list_mon(char ***list, int *n)
{
    char *mon_path;
    struct dirent *dp;
    DIR *dirp;
    struct stat s;
    
    *list = NULL;
    *n    = 0;
    
    mon_path = get_path(NULL, TRUE);
    dirp = opendir(mon_path);
    G_free(mon_path);
    
    if (!dirp)
        return;

    while ((dp = readdir(dirp)) != NULL) {
	int ret;

	if (!dp->d_name[0] || dp->d_name[0] == '.')
	    continue;

	mon_path = get_path(dp->d_name, TRUE);
        ret = G_stat(mon_path, &s);
	G_free(mon_path);

        if (ret != 0 || !S_ISDIR(s.st_mode))
            continue;

        *list = G_realloc(*list, (*n + 1) * sizeof(char *));
        (*list)[*n] = dp->d_name;
        (*n)++;
    }
    closedir(dirp);
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
    char **list;
    int   i, n;

    list_mon(&list, &n);
    
    for (i = 0; i < n; i++)
        if (G_strcasecmp(list[i], name) == 0)
            return TRUE;
    
    return FALSE;
}

/* list related commands for given monitor */
void list_cmd(const char *name, FILE *fd_out)
{
    char *mon_path;
    char cmd_file[GPATH_MAX], buf[4096];
    FILE *fd;
    
    mon_path = get_path(name, FALSE);
    G_file_name(cmd_file, mon_path, "cmd", G_mapset());
    fd = fopen(cmd_file, "r");
    if (!fd)
	G_fatal_error(_("Unable to open file '%s'"), cmd_file);

    while (G_getl2(buf, sizeof(buf) - 1, fd) != 0) {
	fprintf(fd_out, "%s\n", buf);
    }
    
    fclose(fd);

    G_free(mon_path);
}

void list_files(const char *name, FILE *fd_out)
{
    char *p;
    char tmpdir[GPATH_MAX], mon_path[GPATH_MAX];
    struct dirent *dp;
    DIR *dirp;
    
    G_temp_element(tmpdir);
    strcat(tmpdir, "/");
    strcat(tmpdir, "MONITORS");
    strcat(tmpdir, "/");
    strcat(tmpdir, name);
    
    G_file_name(mon_path, tmpdir, NULL, G_mapset());
    fprintf(fd_out, "path=%s\n", mon_path);
    
    dirp = opendir(mon_path);
    if (!dirp)
        G_fatal_error(_("No support files found for monitor <%s>"), name);

    while ((dp = readdir(dirp)) != NULL) {
        if (!dp->d_name[0] || dp->d_name[0] == '.')
            continue;
        
        p = strrchr(dp->d_name, '.');
        if (!p)
            p = dp->d_name;
        else
            p++; /* skip '.' */
        
        fprintf(fd_out, "%s=%s%c%s\n", p,
                mon_path, HOST_DIRSEP, dp->d_name);
    }
}
