
/*****************************************************************************/

/***                                                                       ***/

/***                             open_files()                              ***/

/***   	              Opens input and output raster maps.  		   ***/

/***               Jo Wood, Project ASSIST, 24th January 1993              ***/

/***                                                                       ***/

/*****************************************************************************/

#include <grass/gis.h>
#include <grass/glocale.h>
#include "param.h"


void open_files(void)
{
    /* Open existing file and set the input file descriptor. */

    if ((fd_in = G_open_cell_old(rast_in_name, mapset_in)) < 0)
	G_fatal_error(_("Cannot open raster map <%s>"), rast_in_name);

    /* Open new file and set the output file descriptor. */

    if (mparam != FEATURE) {
	if ((fd_out = G_open_raster_new(rast_out_name, DCELL_TYPE)) < 0)
	    G_fatal_error(_("Cannot create raster map <%s>"), rast_out_name);
    }
    else {
	if ((fd_out = G_open_raster_new(rast_out_name, CELL_TYPE)) < 0)
	    G_fatal_error(_("Cannot create raster map <%s>"), rast_out_name);
    }

    return;
}
