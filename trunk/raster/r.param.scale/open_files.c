
/*****************************************************************************/

/***                                                                       ***/

/***                             open_files()                              ***/

/***   	              Opens input and output raster maps.  		   ***/

/***               Jo Wood, Project ASSIST, 24th January 1993              ***/

/***                                                                       ***/

/*****************************************************************************/

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "param.h"


void open_files(void)
{
    /* Open existing file and set the input file descriptor. */

    fd_in = Rast_open_old(rast_in_name, "");

    /* Open new file and set the output file descriptor. */

    if (mparam != FEATURE)
	fd_out = Rast_open_new(rast_out_name, DCELL_TYPE);
    else
	fd_out = Rast_open_new(rast_out_name, CELL_TYPE);
}
