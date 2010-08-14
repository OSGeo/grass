#include <grass/config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "access.h"

int get_perms(char *path, int *perms, int *group, int *other)
{
    STRUCT_STAT buf;

    if (stat(path, &buf) != 0)
	return -1;

    *perms = buf.st_mode;
    *group = (*perms & GROUP_PERMS) ? 1 : 0;
    *other = (*perms & OTHER_PERMS) ? 1 : 0;
    return 0;
}
