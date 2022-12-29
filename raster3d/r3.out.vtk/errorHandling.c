
/****************************************************************************
*
* MODULE:       r3.out.vtk  
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert at gmx de
* 		27 Feb 2006 Berlin
* PURPOSE:      Converts 3D raster maps (RASTER3D) into the VTK-Ascii format  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include "globalDefs.h"
#include "errorHandling.h"


/* prototypes ************************************************************* */
int close_input_raster3d_map(RASTER3D_Map *map);


/* ************************************************************************* */
/* Error handling ********************************************************** */
/* ************************************************************************* */
void fatal_error(char *errorMsg, input_maps * in)
{
    G_warning("%s", errorMsg);

    /*close all open maps and free memory */
    release_input_maps_struct(in);

    Rast3d_fatal_error("Break because of errors.");
}

/* ************************************************************************* */
/* Close the raster input map ********************************************** */
/* ************************************************************************* */
int CloseInputRasterMap(int fd)
{
    if (fd != -1)
	Rast_close(fd);

    return 0;

}

/* ************************************************************************* */
/* Close the raster g3d input map ****************************************** */
/* ************************************************************************* */
int close_input_raster3d_map(RASTER3D_Map *map)
{
    if (map != NULL) {
	if (!Rast3d_close(map)) {
	    G_warning(_("Unable to close 3D raster map <%s>"), map->fileName);
	    return 1;
	}
    }
    map = NULL;

    return 0;

}

/* ************************************************************************* */
/* Close alls open raster and 3d raster maps and free memory ********************* */
/* ************************************************************************* */
void release_input_maps_struct(input_maps * in)
{
    int error = 0;		/*0 == true, 1 = false */
    int i;

    error += close_input_raster3d_map(in->map);
    error += close_input_raster3d_map(in->map_r);
    error += close_input_raster3d_map(in->map_g);
    error += close_input_raster3d_map(in->map_b);
    error += close_input_raster3d_map(in->map_x);
    error += close_input_raster3d_map(in->map_y);
    error += close_input_raster3d_map(in->map_z);

    error += CloseInputRasterMap(in->top);
    error += CloseInputRasterMap(in->bottom);

    for (i = 0; i < in->numelevmaps; i++) {
	if (in->elevmaps && in->elevmaps[i])
	    error += CloseInputRasterMap(in->elevmaps[i]);
    }

    if (in->elevmaps)
	free(in->elevmaps);

    free(in);

    if (error > 0)
	Rast3d_fatal_error(_("Unable to close input raster maps"));

    return;
}
