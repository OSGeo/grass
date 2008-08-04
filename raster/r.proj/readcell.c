/*
 * readcell.c - reads an entire cell layer into a buffer
 *
 */

#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

FCELL **readcell(int fdi)
{				/* handle of input layer           */
    FCELL **ibuffer;		/* buffer that holds the input map       */
    int nrows;			/* rows of input layer                   */
    int row;			/* counter                               */


    nrows = G_window_rows();

    ibuffer = (FCELL **) G_malloc(sizeof(FCELL **) * nrows);

    G_important_message(_("Allocating memory and reading input map..."));
    G_percent(0, nrows, 5);
    for (row = 0; row < nrows; row++) {
	ibuffer[row] = (FCELL *) G_allocate_raster_buf(FCELL_TYPE);
	if (G_get_raster_row(fdi, ibuffer[row], row, FCELL_TYPE) < 0)
	    G_fatal_error(_("Unable to read raster map row %d"), row);
	G_percent(row, nrows - 1, 5);
    }
    return (ibuffer);
}
