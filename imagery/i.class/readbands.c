#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"


int readbands(int nbands, int cur)
{
    int i;

    for (i = 0; i < nbands; i++)
	Rast_get_c_row_nomask(Bandfd[i], Bandbuf[i], cur);

    return 0;
}
