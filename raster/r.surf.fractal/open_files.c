/*****************************************************************************/
/***                                                                       ***/
/***                             open_files()                              ***/
/***           Opens input and output raster maps for r.example	   ***/
/***                    Jo Wood, V1.0, 13th September, 1994                ***/
/***                                                                       ***/
/*****************************************************************************/

#include <grass/glocale.h>
#include "frac.h"

int 
open_files (void)
{
    /* Open new file and set the output file descriptor. */

    if ( (fd_out=G_open_cell_new(rast_out_name)) <0)
    {
        G_fatal_error (_("ERROR: Problem opening output file."));
    }

    return 0;
}
