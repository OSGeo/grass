
/*****************************************************************************
*
* MODULE:       Grass g3d Library
* AUTHOR(S):    Soeren Gebbert, Braunschweig (GER) Jun 2011
* 		        soerengebbert <at> googlemail <dot> com
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
#include <stdlib.h>
#include <string.h>
#include "test_g3d_lib.h"
#include "grass/interpf.h"

#define EPSILON 0.000000001

static int test_large_file(int depths, int rows, int cols, int tile_size);

/* *************************************************************** */
/* Perfrome the coordinate transformation tests ****************** */
/* *************************************************************** */
int unit_test_put_get_value_large_file(int depths, int rows, int cols, int tile_size)
{
    int sum = 0;

    G_message(_("\n++ Running g3d put/get value large file unit tests ++"));

    sum += test_large_file(depths, rows, cols, tile_size);


    if (sum > 0)
	G_warning(_("\n-- g3d put/get value large file unit tests failure --"));
    else
	G_message(_("\n-- g3d put/get value large file unit tests finished successfully --"));

    return sum;
}

/* *************************************************************** */

int test_large_file(int depths, int rows, int cols, int tile_size)
{
    int sum = 0; 
    int x, y, z;
    DCELL value;
    
    G_message("Testing DCELL put function for large files");
    
    RASTER3D_Region region;
    RASTER3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new g3d map.
     * First we safe the default region. */
    Rast3d_get_window(&region);
    
    region.bottom = -365.5;
    region.top = 365.5;
    region.south = -90;
    region.north = 90;
    region.west = -180;
    region.east = 180;
    region.rows = rows;
    region.cols = cols;
    region.depths = depths;
        
    Rast3d_adjust_region(&region);
        
    G_message("Creating 3D raster map");

    map = Rast3d_open_new_opt_tile_size("test_put_get_value_dcell_large", RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, tile_size);
    
    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);
    
    int count = 1;
    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = count;
                Rast3d_put_value(map, x, y, z, &value, DCELL_TYPE);
                count++;
            }
        }
    }

    G_percent(1, 1, 1);
    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);
         
    G_message("Verifying 3D raster map");

    map = Rast3d_open_cell_old("test_put_get_value_dcell_large", G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XYZ);
    
    count = 1;
    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                Rast3d_get_value(map, x, y, z, &value, DCELL_TYPE);
                if(fabs(value - (double)(count) > EPSILON)) {
                    G_message("At: z %i y %i x %i -- value %.14lf != %.14lf\n", z, y, x, value, (double)(count));
			sum++;
                }
                count++;
            }
        }
    }
    G_percent(1, 1, 1);
    Rast3d_close(map);
    
    G_remove("grid3", "test_put_get_value_dcell_large");
    
    return sum;
}

