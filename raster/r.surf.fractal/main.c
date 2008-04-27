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

#define MAIN                            

#include "frac.h"            
           
int 
main (int argc, char *argv[])       
{
    /*----------------------------------------------------------------------*/
    /*                     GET INPUT FROM USER                              */
    /*----------------------------------------------------------------------*/

    interface(argc,argv);


    /*----------------------------------------------------------------------*/
    /*   		  PROCESS RASTER FILES  		            */
    /*----------------------------------------------------------------------*/

    process();
       
    return EXIT_SUCCESS;
}
