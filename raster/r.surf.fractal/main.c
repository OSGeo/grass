
/****************************************************************************
 *
 * MODULE:       r.surf.fractal
 * AUTHOR(S):    Jo Wood, 19th October, 1994
 * PURPOSE:      GRASS module to manipulate a raster map layer.
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "frac.h"

char
 *rast_out_name,		/* Name of the raster output file.      */
 *mapset_out;

int
  fd_out,			/* File descriptor of output raster     */
  Steps;			/* Number of intermediate images.       */

double H;			/* Hausdorff-Besickovitch dimension.    */

int main(int argc, char *argv[])
{

    interface(argc, argv);
    process();

    return EXIT_SUCCESS;
}
