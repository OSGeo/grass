#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include "local_proto.h"

static char *filename (char *name, char *mapset)
{
    static char path[1024];
    G__file_name (path, "", name, mapset);
    return path;
}

int mapset_permissions (char *mapset)
{
    int stat;

    stat = G__mapset_permissions (mapset);
    if (stat == 1)
    {
	if(access(filename (".lock", mapset), 0) == 0)
	    stat = 0;
    }
    return stat;
}

int mapset_message (char *mapset)
{
    if(printfile(filename (".message", mapset)))
	hit_return();

    return 0;
}

int mapset_question (char *mapset)
{
    if(printfile(filename(".question", mapset)))
	return G_yes("Select this mapset? ", -1);
    return 1;
}

int printfile (char *name)
{
    int fd;
    int n;
    char buf[1024];

    fd = open (name, 0);
    if (fd < 0) return 0;
    while ((n = read (fd, buf, sizeof buf)) > 0)
	write (1, buf, n);
    close (fd);
    return 1;
}
