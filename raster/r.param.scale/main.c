
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

#define MAIN

#include <grass/glocale.h>
#include "param.h"

int main(int argc, char **argv)
{

    /*--------------------------------------------------------------------------*/
    /*                                 INITIALISE                               */

    /*--------------------------------------------------------------------------*/



    /*--------------------------------------------------------------------------*/
    /*                               GET INPUT FROM USER                        */

    /*--------------------------------------------------------------------------*/

    interface(argc, argv);


    /*--------------------------------------------------------------------------*/
    /*                        OPEN INPUT AND OUTPUT RASTER FILES                */

    /*--------------------------------------------------------------------------*/

    /* Make sure that the current projection is not lat/long */
    if ((G_projection() == PROJECTION_LL))
	G_fatal_error(_("Lat/Long location is not supported"));

    open_files();


    /*--------------------------------------------------------------------------*/
    /*                       PROCESS SURFACE FOR FEATURE DETECTION              */

    /*--------------------------------------------------------------------------*/

    process();

    /*--------------------------------------------------------------------------*/
    /*                     CLOSE ALL OPENED FILES AND FREE MEMORY               */

    /*--------------------------------------------------------------------------*/

    close_down();

    if (mparam == FEATURE) {
	write_cols();
	write_cats();
    }

    return 0;
}
