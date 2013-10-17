
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

static int test_put_get_value_dcell(void);
static int test_put_get_value_fcell(void);
static int test_put_get_value_resampling(void);
static int test_get_value_region(RASTER3D_Map *map, int cols, int rows, int depths);
static int test_resampling_dcell(RASTER3D_Map *map, double north, double east, double 
                                 top, int col, int row, int depth, int fact);
static int test_resampling_fcell(RASTER3D_Map *map, double north, double east, double 
                                 top, int col, int row, int depth, int fact);

/* *************************************************************** */
/* Perform the coordinate transformation tests ******************* */
/* *************************************************************** */
int unit_test_put_get_value()
{
    int sum = 0;

    G_message("\n++ Running raster3d put/get value unit tests ++");

    //sum += test_put_get_value_dcell();
    //sum += test_put_get_value_fcell();
    sum += test_put_get_value_resampling();


    if (sum > 0)
	G_warning("\n-- raster3d put/get value unit tests failure --");
    else
	G_message("\n-- raster3d put/get value unit tests finished successfully --");

    return sum;
}

/* *************************************************************** */

int test_put_get_value_dcell(void)
{
    int sum = 0; 
    int x, y, z;
    DCELL value;
    DCELL value_ref;
    
    G_message("Testing DCELL put get value functions");
    
    double north, east, top;
    int col, row, depth;
    
    RASTER3D_Region region;
    RASTER3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new raster3d map.
     * First we safe the default region. */
    Rast3d_get_window(&region);
    
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 1000;
    region.north = 8500;
    region.west = 5000;
    region.east = 10000;
    region.rows = 15;
    region.cols = 10;
    region.depths = 5;
        
    Rast3d_adjust_region(&region);
        
    map = Rast3d_open_new_opt_tile_size("test_put_get_value_dcell", RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, 32);
    
    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);
    /*
     ROWS
  1000 1500 2000 2500 3000 3500 4000 4500 5000 5500 6500 7000 7500 8000 8500 9000 north
    |....|....|....|....|....|....|....|....|....|....|....|....|....|....|....|                         
   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0 region
          
    COLS
  5000 5500 6000 6500 7000 7500 8000 8500 9000 9500 10000 east
    |....|....|....|....|....|....|....|....|....|....|                         
    0    1    2    3    4    5    6    7    8    9   10 region
      
    DEPTHS
    0   200  400  600  800  1000  top
    |....|....|....|....|....|                         
    0    1    2    3    4    5 region                   
    */
    
    for(z = 0; z < region.depths; z++) {
        for(y = 0; y < region.rows; y++) { /* From the north to the south */
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = x + y + z;
                Rast3d_put_value(map, x, y, z, &value, DCELL_TYPE);
            }
        }
    }

    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);

    map = Rast3d_open_cell_old("test_put_get_value_dcell", G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XY);
    
    /* Reread the map and compare the expected results */
    
    G_message("Get the value of the upper left corner -> 0");
    
    
    col = row = depth = 0;
    north = region.north - 0.1; /* north would be out of bounds therefor -0.1 */
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
    
    
    G_message("Get the value of x == y == z == 1 -> x + y + z == 3");
    
    col = row = depth = 1;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 4 y == 3 z == 2 -> x + y + z = 9");
    
    col = 4;
    row = 3;
    depth = 2;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 9 y == 14 z == 4 -> x + y + z = 27");
    
    col = 9;
    row = 14;
    depth = 4;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
        
    G_message("Get the value of x == 10 y == 15 z == 5 -> x + y + z = NAN");
        
    col = 10;
    row = 15;
    depth = 5;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    Rast3d_get_region_value(map, north, east, top, &value, DCELL_TYPE);
    Rast3d_get_value(map, col, row, depth, &value_ref, DCELL_TYPE);
    /* Rast3d_get_value_region does not work with coordinates outside the region */
    printf("Value %g == %g\n", value, value_ref);
    
    if(value == 0 || value < 0 || value > 0) {
        G_message("Error in Rast3d_get_region_value");
        sum++;
    }
    if(value_ref == 0 || value_ref < 0 || value_ref > 0) {
        G_message("Error in Rast3d_get_value");
        sum++;
    }
    
    Rast3d_close(map);
    
    G_remove("grid3", "test_put_get_value_dcell");
    
    return sum;
}

/* *************************************************************** */

int test_put_get_value_fcell(void)
{
     int sum = 0; 
    int x, y, z;
    FCELL value;
    FCELL value_ref;
    
    G_message("Testing FCELL put get value functions");
    
    double north, east, top;
    int col, row, depth;
    
    RASTER3D_Region region;
    RASTER3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new raster3d map.
     * First we safe the default region. */
    Rast3d_get_window(&region);
    
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 1000;
    region.north = 8500;
    region.west = 5000;
    region.east = 10000;
    region.rows = 15;
    region.cols = 10;
    region.depths = 5;
        
    Rast3d_adjust_region(&region);
        
    map = Rast3d_open_new_opt_tile_size("test_put_get_value_fcell", RASTER3D_USE_CACHE_XY, &region, FCELL_TYPE, 32);
    
    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);
    
    for(z = 0; z < region.depths; z++) {
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = x + y + z;
                Rast3d_put_value(map, x, y, z, &value, FCELL_TYPE);
            }
        }
    }

    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);

    map = Rast3d_open_cell_old("test_put_get_value_fcell", G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XY);
    
       /* Reread the map and compare the expected results */
    
    G_message("Get the value of the lower left corner -> 0");
    
    col = row = depth = 0;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == y == z == 1 -> x + y + z == 3");
    
    col = row = depth = 1;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 4 y == 3 z == 2 -> x + y + z = 9");
    
    col = 4;
    row = 3;
    depth = 2;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 9 y == 14 z == 4 -> x + y + z = 27");
    
    col = 9;
    row = 14;
    depth = 4;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
        
    G_message("Get the value of x == 10 y == 15 z == 5 -> x + y + z = NAN");
    
    col = 10;
    row = 15;
    depth = 5;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    Rast3d_get_region_value(map, north, east, top, &value, FCELL_TYPE);
    Rast3d_get_value(map, 10, 15, 5, &value_ref, FCELL_TYPE);
    /* Rast3d_get_value_region does not work with coordinates outside the region */
    printf("Value %g == %g\n", value, value_ref);
    
    if(value == 0 || value < 0 || value > 0) {
        G_message("Error in Rast3d_get_region_value");
        sum++;
    }
    if(value_ref == 0 || value_ref < 0 || value_ref > 0) {
        G_message("Error in Rast3d_get_value");
        sum++;
    }
    
    Rast3d_close(map);
    
    G_remove("grid3", "test_put_get_value_fcell");
    
    return sum;
}

/* *************************************************************** */

int test_put_get_value_resampling(void)
{
    int sum = 0; 
    int x, y, z;
    DCELL value;
    
    G_message("Testing put get resample value functions");
    
    double north, east, top;
    int col, row, depth;
    
    RASTER3D_Region region;
    RASTER3D_Region window;
    RASTER3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new raster3d map.
     * First we safe the default region. */
    Rast3d_get_window(&region);
    
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 1000;
    region.north = 8500;
    region.west = 5000;
    region.east = 10000;
    region.rows = 15;
    region.cols = 10;
    region.depths = 5;
        
    Rast3d_adjust_region(&region);
    
    map = Rast3d_open_new_opt_tile_size("test_put_get_value_resample", RASTER3D_USE_CACHE_XY, &region, DCELL_TYPE, 32);
    /*
     ROWS
  1000 1500 2000 2500 3000 3500 4000 4500 5000 5500 6500 7000 7500 8000 8500 9000 north
    |....|....|....|....|....|....|....|....|....|....|....|....|....|....|....|                         
   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0 region
    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
   30   28   26   24   22   20   18   16   14   12   10    8    6    4    2    0 window
          
    COLS
  5000 5500 6000 6500 7000 7500 8000 8500 9000 9500 10000 east
    |....|....|....|....|....|....|....|....|....|....|                         
    0    1    2    3    4    5    6    7    8    9   10 region
    |    |    |    |    |    |    |    |    |    |    |
    0    2    4    6    8   10   12   14   16   18   20 window
    DEPTHS
    0   200  400  600  800  1000 top
    |....|....|....|....|....|                         
    0    1    2    3    4    5 region  
    |    |    |    |    |    |
    0    2    4    6    8   10 window                   
    */
    
    for(z = 0; z < region.depths; z++) {
        for(y = 0; y < region.rows; y++) {  /* North to south */
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = x + y + z;
                Rast3d_put_double(map, x, y, z, value);
            }
        }
    }

    /* Write everything to the disk */
    Rast3d_flush_all_tiles(map);
    Rast3d_close(map);

    /* We modify the window for resampling tests */
    Rast3d_region_copy(&window, &region);

    /* Double the cols, rows and depths -> 8x resolution window */
    window.rows = 30;
    window.cols = 20;
    window.depths = 10;

    Rast3d_adjust_region(&window);

    map = Rast3d_open_cell_old("test_put_get_value_resample", G_mapset(), &region, DCELL_TYPE, RASTER3D_USE_CACHE_XY);

    /* The window has the 8x resolution as the map region */
    Rast3d_set_window_map(map, &window);
    
    /* Reread the map and compare the expected results */
    
    G_message("Get the value of the upper left corner -> 0");
    
    col = row = depth = 0;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G_message("Get the value of x == y == z == 1 -> x + y + z == 3");
    
    
    col = row = depth = 1;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G_message("Get the value of x == 7 y == 9 z == 3 -> x + y + z == 19");
        
    col = 7;
    row = 9;
    depth = 3;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G_message("Get the value of x == 9 y == 14 z == 4 -> x + y + z == 27");
        
    col = 9;
    row = 14;
    depth = 4;
    north = region.north - region.ns_res * (row + 1);
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    sum += test_get_value_region(map, region.cols, region.rows, region.depths);
    
    Rast3d_close(map);
    
    G_remove("grid3", "test_put_get_value_resample");
    
    return sum;
}

/* *************************************************************** */

int test_resampling_dcell(RASTER3D_Map *map, double north, double east, double top, int col, int row, int depth, int fact)
{
    int sum = 0;
    DCELL value;
    DCELL value_ref;
    DCELL value_reg;
    DCELL value_win;
    
    Rast3d_get_region_value(map, north, east, top, &value, DCELL_TYPE);
    Rast3d_get_window_value(map, north, east, top, &value_win, DCELL_TYPE);
    Rast3d_get_value(map, col * fact, row * fact, depth * fact, &value_ref, DCELL_TYPE);
    Rast3d_get_value_region(map, col, row, depth, &value_reg, DCELL_TYPE);
    printf("Value %g == %g == %g == %g\n", value, value_win, value_ref, value_reg);
    
    if(value != col + row + depth) {
        G_message("Error in Rast3d_get_region_value");
        sum++;
    }
    if(value_win != col + row + depth) {
        G_message("Error in Rast3d_get_window_value");
        sum++;
    }
    if(value_ref != col + row + depth) {
        G_message("Error in Rast3d_get_value");
        sum++;
    }
    if(value_reg != col + row + depth) {
        G_message("Error in Rast3d_get_value_region");
        sum++;
    }
    
    return sum;
}

/* *************************************************************** */

int test_resampling_fcell(RASTER3D_Map *map, double north, double east, double top, int col, int row, int depth, int fact)
{
    int sum = 0;
    FCELL value;
    FCELL value_ref;
    FCELL value_reg;
    FCELL value_win;
    
    Rast3d_get_region_value(map, north, east, top, &value, FCELL_TYPE);
    Rast3d_get_window_value(map, north, east, top, &value_win, FCELL_TYPE);
    Rast3d_get_value(map, col * fact, row * fact, depth * fact, &value_ref, FCELL_TYPE);
    Rast3d_get_value_region(map, col, row, depth, &value_reg, FCELL_TYPE);
    printf("Value %g == %g == %g == %g\n", value, value_win, value_ref, value_reg);
    
    if(value != col + row + depth) {
        G_message("Error in Rast3d_get_region_value");
        sum++;
    }
    if(value_win != col + row + depth) {
        G_message("Error in Rast3d_get_window_value");
        sum++;
    }
    if(value_ref != col + row + depth) {
        G_message("Error in Rast3d_get_value");
        sum++;
    }
    if(value_reg != col + row + depth) {
        G_message("Error in Rast3d_get_value_region");
        sum++;
    }
    
    return sum;
}

/* *************************************************************** */

int test_get_value_region(RASTER3D_Map *map, int cols, int rows, int depths)
{
    int sum = 0;
    FCELL fvalue1 = 0.0;
    FCELL fvalue2 = 0.0;
    DCELL dvalue1 = 0.0;
    DCELL dvalue2 = 0.0;
    
    /* Test for correct Null value */
    Rast3d_get_value_region(map, -1, -1, -1, &fvalue1, FCELL_TYPE);
    Rast3d_get_value_region(map, cols, rows, depths, &fvalue2, FCELL_TYPE);
    Rast3d_get_value_region(map, -1, -1, -1, &dvalue1, DCELL_TYPE);
    Rast3d_get_value_region(map, cols, rows, depths, &dvalue2, DCELL_TYPE);
    printf("Value %g == %g == %g == %g\n", fvalue1, fvalue2, dvalue1, dvalue2);
    
    if(!Rast_is_f_null_value(&fvalue1)) {
        G_message("Error in Rast3d_get_value_region");
        sum++;
    }
    if(!Rast_is_f_null_value(&fvalue2)) {
        G_message("Error in Rast3d_get_value_region");
        sum++;
    }
    if(!Rast_is_d_null_value(&dvalue1)) {
        G_message("Error in Rast3d_get_value_region");
        sum++;
    }
    if(!Rast_is_d_null_value(&dvalue2)) {
        G_message("Error in Rast3d_get_value_region");
        sum++;
    }
    
    return sum;
}
