
/*****************************************************************************
*
* MODULE:       Grass raster3d Library
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
#include "test_raster3d_lib.h"
#include "grass/interpf.h"

#define EPSILON 0.000000001
#define RAND_VALUE_VECTOR_SIZE 1000

static int test_large_file(int depths, int rows, int cols, int tile_size);
static int test_large_file_zeros(int depths, int rows, int cols, int tile_size);
static int test_large_file_random(int depths, int rows, int cols, int tile_size);
static int test_large_file_sparse_random(int depths, int rows, int cols, int tile_size);

/* *************************************************************** */
/* Perform the coordinate transformation tests ****************** */
/* *************************************************************** */
int unit_test_put_get_value_large_file(int depths, int rows, int cols, int tile_size)
{
    int sum = 0;

    G_message(_("\n++ Running raster3d put/get value large file unit tests ++"));

    sum += test_large_file_random(depths, rows, cols, tile_size);
    sum += test_large_file_sparse_random(depths, rows, cols, tile_size);
    sum += test_large_file(depths, rows, cols, tile_size);
    sum += test_large_file(depths, rows, cols, tile_size);
    sum += test_large_file(depths, rows, cols, tile_size);
    sum += test_large_file_zeros(depths, rows, cols, tile_size);


    if (sum > 0)
	G_warning(_("\n-- raster3d put/get value large file unit tests failure --"));
    else
	G_message(_("\n-- raster3d put/get value large file unit tests finished successfully --"));

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
    
    /* We need to set up a specific region for the new raster3d map.
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

    map = Rast3d_open_new_opt_tile_size("test_put_get_value_dcell_large",
    		RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, tile_size);
    
    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);
    
    int count = 1;
    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
              /* Put the counter as cell value */
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

    map = Rast3d_open_cell_old("test_put_get_value_dcell_large",
    		G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XYZ);
    
    count = 1;
    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Check the counter as cell value */
                Rast3d_get_value(map, x, y, z, &value, DCELL_TYPE);
                if(fabs(value - (double)(count) > EPSILON)) {
                    G_message("At: z %i y %i x %i -- value %.14lf != %.14lf\n",
                    		z, y, x, value, (double)(count));
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


/* *************************************************************** */

int test_large_file_zeros(int depths, int rows, int cols, int tile_size)
{
    int sum = 0;
    int x, y, z;
    DCELL value;

    G_message("Testing DCELL put function for large files filled with zeros");

    RASTER3D_Region region;
    RASTER3D_Map *map = NULL;

    /* We need to set up a specific region for the new raster3d map.
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

    G_message("Creating 3D raster map filled with zeros");

    map = Rast3d_open_new_opt_tile_size("test_put_get_value_dcell_large_zeros",
    		RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, tile_size);

    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);

    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
              /* Put the counter as cell value */
                value = 0.0;
                Rast3d_put_value(map, x, y, z, &value, DCELL_TYPE);
            }
        }
    }

    G_percent(1, 1, 1);
    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);

    G_message("Verifying 3D raster map filled with zeros");

    map = Rast3d_open_cell_old("test_put_get_value_dcell_large_zeros",
    		G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XY);

    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Check the counter as cell value */
                Rast3d_get_value(map, x, y, z, &value, DCELL_TYPE);
                if(value > EPSILON) {
                    G_message("At: z %i y %i x %i -- value %.14lf != 0.0\n",
                    		z, y, x, value);
                    sum++;
                }
            }
        }
    }
    G_percent(1, 1, 1);
    Rast3d_close(map);

    G_remove("grid3", "test_put_get_value_dcell_large_zeros");

    return sum;
}

/* *************************************************************** */

int test_large_file_random(int depths, int rows, int cols, int tile_size)
{
    int sum = 0;
    int x, y, z, i;
    DCELL value, random_value;
    DCELL *random_value_vector = G_calloc(RAND_VALUE_VECTOR_SIZE, sizeof(DCELL));


    G_message("Testing DCELL put function for large files filled with random values");

    RASTER3D_Region region;
    RASTER3D_Map *map = NULL;

    /* We need to set up a specific region for the new raster3d map.
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

    G_message("Creating 3D raster map filled with random values");

    map = Rast3d_open_new_opt_tile_size("test_put_get_value_dcell_large_random",
    		RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, tile_size);

    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);

    srand(1);
    /* We fill the random value vector */
    for(i = 0; i < RAND_VALUE_VECTOR_SIZE; i++) {
    	random_value_vector[i] = (DCELL)rand();
    }

    i = 0;

    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
              /* Put the counter as cell value */
                value = random_value_vector[i];
                Rast3d_put_value(map, x, y, z, &value, DCELL_TYPE);
                i++;
                if(i == RAND_VALUE_VECTOR_SIZE)
                	i = 0;
            }
        }
    }

    G_percent(1, 1, 1);
    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);

    G_message("Verifying 3D raster map filled with random values");

    map = Rast3d_open_cell_old("test_put_get_value_dcell_large_random",
    		G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XY);

    i = 0;

    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Check the counter as cell value */
                Rast3d_get_value(map, x, y, z, &value, DCELL_TYPE);
                random_value = random_value_vector[i];
                if(fabs(value - random_value) > EPSILON) {
                    G_message("At: z %i y %i x %i -- value %.14lf != %.14lf\n",
                    		z, y, x, value, random_value);
                    sum++;
                }
                i++;
                if(i == RAND_VALUE_VECTOR_SIZE)
                	i = 0;
            }
        }
    }
    G_percent(1, 1, 1);
    Rast3d_close(map);

    G_free(random_value_vector);

    G_remove("grid3", "test_put_get_value_dcell_large_random");

    return sum;
}

/* *************************************************************** */

int test_large_file_sparse_random(int depths, int rows, int cols, int tile_size)
{
    int sum = 0;
    int x, y, z, i;
    DCELL value, random_value;
    DCELL *random_value_vector = G_calloc(RAND_VALUE_VECTOR_SIZE, sizeof(DCELL));

    G_message("Testing DCELL put function for large files filled with sparse random values");

    RASTER3D_Region region;
    RASTER3D_Map *map = NULL;

    /* We need to set up a specific region for the new raster3d map.
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

    G_message("Creating 3D raster map filled with sparse random values");

    map = Rast3d_open_new_opt_tile_size("test_put_get_value_dcell_large_sparse_random",
    		RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, tile_size);

    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);

    srand(1);

    /* We fill the random value vector */
    for(i = 0; i < RAND_VALUE_VECTOR_SIZE; i++) {
    	/* Put the counter as cell value */
        value = (DCELL)rand();
        value /= RAND_MAX;
        if(value <= 0.7)
        	value = 0.0;
        else if(value <= 0.8)
        	value = 1.0;
        else if(value <= 0.9)
        	value = 2.0;
        else if(value <= 1.0)
        	value = 3.0;
        else
        	value = 4.0;
    	random_value_vector[i] = value;
    }

    i = 0;

    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Put the counter as cell value */
                  value = random_value_vector[i];
                  Rast3d_put_value(map, x, y, z, &value, DCELL_TYPE);
                  i++;
                  if(i == RAND_VALUE_VECTOR_SIZE)
                  	i = 0;
            }
        }
    }

    G_percent(1, 1, 1);
    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);

    G_message("Verifying 3D raster map filled with sparse random values");

    map = Rast3d_open_cell_old("test_put_get_value_dcell_large_sparse_random",
    		G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XY);

    i = 0;

    for(z = 0; z < region.depths; z++) {
	G_percent(z, region.depths, 1);
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Check the counter as cell value */
                Rast3d_get_value(map, x, y, z, &value, DCELL_TYPE);
                if(fabs(value - random_value_vector[i]) > EPSILON) {
                    G_message("At: z %i y %i x %i -- value %.14lf != %.14lf\n",
                    		z, y, x, value, random_value);
                    sum++;
                }
                i++;
                if(i == RAND_VALUE_VECTOR_SIZE)
                	i = 0;
            }
        }
    }
    G_percent(1, 1, 1);
    Rast3d_close(map);

    G_free(random_value_vector);

    G_remove("grid3", "test_put_get_value_dcell_large_sparse_random");

    return sum;
}

