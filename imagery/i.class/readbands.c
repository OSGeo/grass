#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"


int readbands(int nbands, int cur)
{
    register int i;

    for (i = 0; i < nbands; i++)
	if (G_get_map_row_nomask(Bandfd[i], Bandbuf[i], cur) < 0)
	    G_fatal_error(_("Error reading raster map in function readbands."));

    return 0;
}
