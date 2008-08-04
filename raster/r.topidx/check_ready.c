#include "global.h"
#include <grass/glocale.h>

int check_ready(void)
{
    int retval;

    retval = 0;

    if (!G_find_file("cell", iname, mapset)) {
	G_fatal_error(_("Raster map <%s> not found"), iname);
	retval = 1;
    }

    return retval;
}
