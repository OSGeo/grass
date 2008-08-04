#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"


/* open and allocate space for the subgroup band files */
int open_band_files(void)
{
    int n, nbands;
    char *name, *mapset;

    /* allocate row buffers and open raster maps */
    nbands = Refer.nfiles;
    Bandbuf = (CELL **) G_malloc(nbands * sizeof(CELL *));
    Bandfd = (int *)G_malloc(nbands * sizeof(int));
    for (n = 0; n < nbands; n++) {
	Bandbuf[n] = G_allocate_cell_buf();
	name = Refer.file[n].name;
	mapset = Refer.file[n].mapset;
	if ((Bandfd[n] = G_open_cell_old(name, mapset)) < 0)
	    G_fatal_error(_("Unable to open band files."));
    }

    return 0;
}


/* close and free space for the subgroup band files */
int close_band_files(void)
{
    int n, nbands;

    nbands = Refer.nfiles;
    for (n = 0; n < nbands; n++) {
	G_free(Bandbuf[n]);
	G_close_cell(Bandfd[n]);
    }
    G_free(Bandbuf);
    G_free(Bandfd);

    return 0;
}
