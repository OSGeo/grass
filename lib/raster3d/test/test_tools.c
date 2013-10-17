
/*****************************************************************************
*
* MODULE:       Grass raster3d Library
* AUTHOR(S):    Soeren Gebbert, Braunschweig (GER) Jun 2011
* 	            soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:	Unit and Integration tests
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "test_raster3d_lib.h"
#include <sys/time.h>

/* *************************************************************** */
/* Compute the difference between two time steps ***************** */

/* *************************************************************** */
double compute_time_difference(struct timeval start, struct timeval end) {
    int sec;
    int usec;

    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;

    return (double) sec + (double) usec / 1000000;
}
