
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

static int test_put_get_value_dcell(void);
static int test_put_get_value_fcell(void);
static int test_put_get_value_resampling(void);
static int test_resampling_dcell(G3D_Map *map, double north, double east, double 
                                 top, int col, int row, int depth, int fact);
static int test_resampling_fcell(G3D_Map *map, double north, double east, double 
                                 top, int col, int row, int depth, int fact);

/* *************************************************************** */
/* Perfrome the coordinate transformation tests ****************** */
/* *************************************************************** */
int unit_test_put_get_value(void)
{
    int sum = 0;

    G_message(_("\n++ Running g3d put/get value unit tests ++"));

    sum += test_put_get_value_dcell();
    sum += test_put_get_value_fcell();
    sum += test_put_get_value_resampling();

    if (sum > 0)
	G_warning(_("\n-- g3d put/get value unit tests failure --"));
    else
	G_message(_("\n-- g3d put/get value unit tests finished successfully --"));

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
    
    G3D_Region region;
    G3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new g3d map.
     * First we safe the default region. */
    G3d_getWindow(&region);
    
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 1000;
    region.north = 8500;
    region.west = 5000;
    region.east = 10000;
    region.rows = 15;
    region.cols = 10;
    region.depths = 5;
        
    G3d_adjustRegion(&region);
        
    map = G3d_openNewOptTileSize("test_put_get_value_dcell", G3D_USE_CACHE_XY, &region, DCELL_TYPE, 32);
    
    /* The window is the same as the map region ... of course */
    G3d_setWindowMap(map, &region);
    /*
     ROWS
  1000 1500 2000 2500 3000 3500 4000 4500 5000 5500 6500 7000 7500 8000 8500 9000 north
    |....|....|....|....|....|....|....|....|....|....|....|....|....|....|....|                         
    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15 region
          
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
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = x + y + z;
                G3d_putValue(map, x, y, z, &value, DCELL_TYPE);
            }
        }
    }
    /* Write everything to the disk */
    G3d_flushAllTiles(map);
    
    /* Reread the map and compare the expected results */
    
    G_message("Get the value of the lower left corner -> 0");
    
    north = region.south;
    east = region.west;
    top = region.bottom;
    
    col = row = depth = 0;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == y == z == 1 -> x + y + z == 3");
    
    col = row = depth = 1;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 4 y == 3 z == 2 -> x + y + z = 9");
    
    col = 4;
    row = 3;
    depth = 2;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 9 y == 14 z == 4 -> x + y + z = 27");
    
    col = 9;
    row = 14;
    depth = 4;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 1);
        
    G_message("Get the value of x == 10 y == 15 z == 5 -> x + y + z = NAN");
        
    col = 10;
    row = 15;
    depth = 5;
    north = region.south + region.ns_res * 15;
    east = region.west + region.ew_res * 10;
    top = region.bottom + region.tb_res * 5;
    
    G3d_getRegionValue(map, north, east, top, &value, DCELL_TYPE);
    G3d_getValue(map, col, row, depth, &value_ref, DCELL_TYPE);
    /* G3d_getValueRegion does not work with coordinates outside the region */
    printf("Value %g == %g\n", value, value_ref);
    
    if(value == 0 || value < 0 || value > 0) {
        G_message("Error in G3d_getRegionValue");
        sum++;
    }
    if(value_ref == 0 || value_ref < 0 || value_ref > 0) {
        G_message("Error in G3d_getValue");
        sum++;
    }
    
    G3d_closeCell(map);
    
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
    
    G3D_Region region;
    G3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new g3d map.
     * First we safe the default region. */
    G3d_getWindow(&region);
    
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 1000;
    region.north = 8500;
    region.west = 5000;
    region.east = 10000;
    region.rows = 15;
    region.cols = 10;
    region.depths = 5;
        
    G3d_adjustRegion(&region);
        
    map = G3d_openNewOptTileSize("test_put_get_value_dcell", G3D_USE_CACHE_XY, &region, FCELL_TYPE, 32);
    
    /* The window is the same as the map region ... of course */
    G3d_setWindowMap(map, &region);
    
    for(z = 0; z < region.depths; z++) {
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = x + y + z;
                G3d_putValue(map, x, y, z, &value, FCELL_TYPE);
            }
        }
    }
    /* Write everything to the disk */
    G3d_flushAllTiles(map);
    
       /* Reread the map and compare the expected results */
    
    G_message("Get the value of the lower left corner -> 0");
    
    north = region.south;
    east = region.west;
    top = region.bottom;
    
    col = row = depth = 0;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == y == z == 1 -> x + y + z == 3");
    
    col = row = depth = 1;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 4 y == 3 z == 2 -> x + y + z = 9");
    
    col = 4;
    row = 3;
    depth = 2;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
    
    G_message("Get the value of x == 9 y == 14 z == 4 -> x + y + z = 27");
    
    col = 9;
    row = 14;
    depth = 4;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_fcell(map, north, east, top, col, row, depth, 1);
        
    G_message("Get the value of x == 10 y == 15 z == 5 -> x + y + z = NAN");
    
    north = region.south + region.ns_res * 15;
    east = region.west + region.ew_res * 10;
    top = region.bottom + region.tb_res * 5;
    
    G3d_getRegionValue(map, north, east, top, &value, FCELL_TYPE);
    G3d_getValue(map, 10, 15, 5, &value_ref, FCELL_TYPE);
    /* G3d_getValueRegion does not work with coordinates outside the region */
    printf("Value %g == %g\n", value, value_ref);
    
    if(value == 0 || value < 0 || value > 0) {
        G_message("Error in G3d_getRegionValue");
        sum++;
    }
    if(value_ref == 0 || value_ref < 0 || value_ref > 0) {
        G_message("Error in G3d_getValue");
        sum++;
    }
    
    G3d_closeCell(map);
    
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
    
    G3D_Region region;
    G3D_Region window;
    G3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new g3d map.
     * First we safe the default region. */
    G3d_getWindow(&region);
    
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 1000;
    region.north = 8500;
    region.west = 5000;
    region.east = 10000;
    region.rows = 15;
    region.cols = 10;
    region.depths = 5;
        
    G3d_adjustRegion(&region);
    
    map = G3d_openNewOptTileSize("test_put_get_value_resample", G3D_USE_CACHE_XY, &region, DCELL_TYPE, 32);
    
    /* We modify the window for resampling tests */
    G3d_regionCopy(&window, &region);
        
    /* Double the cols, rows and depths -> 8x resolution window */
    window.rows = 30;
    window.cols = 20;
    window.depths = 10;
    
    G3d_adjustRegion(&window);
    
    /* The window is the same as the map region ... of course */
    G3d_setWindowMap(map, &window);
    /*
     ROWS
  1000 1500 2000 2500 3000 3500 4000 4500 5000 5500 6500 7000 7500 8000 8500 9000 north
    |....|....|....|....|....|....|....|....|....|....|....|....|....|....|....|                         
    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15 region
    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
    0    2    4    6    8   10   12   14   16   18   20   22   24   26   28   30 window
          
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
        for(y = 0; y < region.rows; y++) {
            for(x = 0; x < region.cols; x++) {
                /* Add cols, rows and depths and put this in the map */
                value = x + y + z;
                G3d_putDouble(map, x, y, z, value);
            }
        }
    }
    /* Write everything to the disk */
    G3d_flushAllTiles(map);
    
    /* Reread the map and compare the expected results */
    
    G_message("Get the value of the lower left corner -> 0");
    
    north = region.south;
    east = region.west;
    top = region.bottom;
    
    col = row = depth = 0;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G_message("Get the value of x == y == z == 1 -> x + y + z == 3");
    
    
    col = row = depth = 1;
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G_message("Get the value of x == 7 y == 9 z == 3 -> x + y + z == 19");
        
    col = 7;
    row = 9;
    depth = 3;
    
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G_message("Get the value of x == 9 y == 14 z == 4 -> x + y + z == 27");
        
    col = 9;
    row = 14;
    depth = 4;
    
    north = region.south + region.ns_res * row;
    east = region.west + region.ew_res * col;
    top = region.bottom + region.tb_res * depth;
    
    sum += test_resampling_dcell(map, north, east, top, col, row, depth, 2);
    
    G3d_closeCell(map);
    
    G_remove("grid3", "test_put_get_value_dcell");
    
    return sum;
}

/* *************************************************************** */

int test_resampling_dcell(G3D_Map *map, double north, double east, double top, int col, int row, int depth, int fact)
{
    int sum = 0;
    DCELL value;
    DCELL value_ref;
    DCELL value_reg;
    DCELL value_win;
    
    G3d_getRegionValue(map, north, east, top, &value, DCELL_TYPE);
    G3d_getWindowValue(map, north, east, top, &value_win, DCELL_TYPE);
    G3d_getValue(map, col * fact, row * fact, depth * fact, &value_ref, DCELL_TYPE);
    G3d_getValueRegion(map, col, row, depth, &value_reg, DCELL_TYPE);
    printf("Value %g == %g == %g == %g\n", value, value_win, value_ref, value_reg);
    
    if(value != col + row + depth) {
        G_message("Error in G3d_getRegionValue");
        sum++;
    }
    if(value != col + row + depth) {
        G_message("Error in G3d_getWindowValue");
        sum++;
    }
    if(value != col + row + depth) {
        G_message("Error in G3d_getValue");
        sum++;
    }
    if(value != col + row + depth) {
        G_message("Error in G3d_getValueRegion");
        sum++;
    }
    
    return sum;
}

/* *************************************************************** */

int test_resampling_fcell(G3D_Map *map, double north, double east, double top, int col, int row, int depth, int fact)
{
    int sum = 0;
    FCELL value;
    FCELL value_ref;
    FCELL value_reg;
    FCELL value_win;
    
    G3d_getRegionValue(map, north, east, top, &value, FCELL_TYPE);
    G3d_getWindowValue(map, north, east, top, &value_win, FCELL_TYPE);
    G3d_getValue(map, col * fact, row * fact, depth * fact, &value_ref, FCELL_TYPE);
    G3d_getValueRegion(map, col, row, depth, &value_reg, FCELL_TYPE);
    printf("Value %g == %g == %g == %g\n", value, value_win, value_ref, value_reg);
    
    if(value != col + row + depth) {
        G_message("Error in G3d_getRegionValue");
        sum++;
    }
    if(value != col + row + depth) {
        G_message("Error in G3d_getWindowValue");
        sum++;
    }
    if(value != col + row + depth) {
        G_message("Error in G3d_getValue");
        sum++;
    }
    if(value != col + row + depth) {
        G_message("Error in G3d_getValueRegion");
        sum++;
    }
    
    return sum;
}