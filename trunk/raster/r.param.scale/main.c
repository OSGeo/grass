
/****************************************************************************
 *
 * MODULE:       r.param.scale
 * AUTHOR(S):    Jo Wood, V 1.1, 11th December, 1994 (original contributor)
 * PURPOSE:      GRASS module for extracting multi-scale surface parameters.
 * COPYRIGHT:    (C) 1999-2004 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/glocale.h>
#include "param.h"

const char *rast_in_name;	/* Name of the raster file to process.  */
const char *rast_out_name;	/* Name of the raster output file.      */
int constrained;		/* Flag that forces quadtratic through  */
				/* the central cell of the window.      */
int
  fd_in,			/* File descriptor for input and        */
  fd_out,			/* output raster files.                 */
  wsize,			/* Size of local processing window.     */
  mparam;			/* Morphometric parameter to calculate. */


double
  resoln,			/* Planimetric resolution.              */
  exponent,			/* Distance weighting exponent.         */
  zscale,			/* Vertical scaling factor.             */
  slope_tol,			/* Vertical tolerences for surface      */
  curve_tol;			/* feature identification.              */

int main(int argc, char **argv)
{
    interface(argc, argv);

    /* Make sure that the current projection is not lat/long */
    if ((G_projection() == PROJECTION_LL))
	G_fatal_error(_("Lat/Long locations are not supported by this module"));

    open_files();

    process();

    close_down();

    if (mparam == FEATURE) {
	write_cols();
	write_cats();
    }

    return 0;
}
