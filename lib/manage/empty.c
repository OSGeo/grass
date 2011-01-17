/* look for at least one file in the element */
#include <sys/types.h>
#include <dirent.h>
#include <grass/gis.h>

int empty(char *elem)
{
    DIR *dirp;
    struct dirent *dp;
    char dir[1024];
    int any;

    G_file_name(dir, elem, "", G_mapset());

    any = 0;
    if ((dirp = opendir(dir)) != NULL) {
	while (!any && (dp = readdir(dirp)) != NULL) {
	    if (dp->d_name[0] != '.')
		any = 1;
	}
	closedir(dirp);
    }

    return any == 0;
}
