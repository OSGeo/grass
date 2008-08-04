#include <unistd.h>
#include "local_proto.h"
#include <grass/glocale.h>


void check_ready(void)
{
    FILE *fp;

    mapset = G_find_cell(iname, "");
    if (!mapset)
	G_fatal_error(_("Raster map <%s> not found"), iname);

    fp = fopen(file, "r");
    if (!fp)
	return;

    fclose(fp);

    if (overwr)
	unlink(file);
    else
	G_fatal_error("%s - file already exists", file);
}
