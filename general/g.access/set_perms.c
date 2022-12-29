#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "access.h"
#include "local_proto.h"

int set_perms(char *path, int perms, int group, int other)
{
    char *explain_perms();

    perms |= OWNER_PERMS;

    perms &= ~GROUP_BITS;
    perms &= ~OTHER_BITS;

    if (group)
	perms |= GROUP_PERMS;
    if (other)
	perms |= OTHER_PERMS;

    if (chmod(path, perms) == 0)
	fprintf(stdout, "%s\n", explain_perms(group, other, 0));
    else
	G_fatal_error(_("Unable to change mapset permissions"));

    return 0;
}
