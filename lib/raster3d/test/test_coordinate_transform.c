#include <stdlib.h>
#include <string.h>
#include "test_raster3d_lib.h"
#include "grass/interpf.h"

static int test_coordinate_transform(void);
static int test_region(void);


/* *************************************************************** */
/* Perfrome the coordinate transformation tests ****************** */
/* *************************************************************** */
int unit_test_coordinate_transform(void)
{
    int sum = 0;

    G_message("\n++ Running raster3d coordinate transform unit tests ++");

    sum += test_coordinate_transform();
    sum += test_region();

    if (sum > 0)
	G_warning("\n-- raster3d coordinate transform unit tests failure --");
    else
	G_message("\n-- raster3d coordinate transform unit tests finished successfully --");

    return sum;
}

/* *************************************************************** */

int test_coordinate_transform(void)
{
    int sum = 0; 
    
    double north, east, top;
    int col = 0, row = 0, depth = 0;
    
    RASTER3D_Region region, default_region;
    RASTER3D_Map *map = NULL;
    
    /* We need to set up a specific region for the new raster3d map.
     * First we safe the default region. */
    Rast3d_get_window(&default_region);
    Rast3d_region_copy(&region, &default_region);
    
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
    
    map = Rast3d_open_new_opt_tile_size("test_coordinate_transform", RASTER3D_USE_CACHE_XYZ, &region, FCELL_TYPE, 32);
        
    /* The window is the same as the map region ... of course */
    Rast3d_set_window_map(map, &region);
    
    G_message("Test the upper right corner, coordinates must be col = 9, row = 0, depth = 4");
    
    /*
     ROWS
  1000 1500 2000 2500 3000 3500 4000 4500 5000 5500 6500 7000 7500 8000 8500 9000
    |....|....|....|....|....|....|....|....|....|....|....|....|....|....|....|                         
   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
          
    COLS
  5000 5500 6000 6500 7000 7500 8000 8500 9000 9500 10000
    |....|....|....|....|....|....|....|....|....|....|                         
    0    1    2    3    4    5    6    7    8    9   10
      
    DEPTHS
    0   200  400  600  800  1000
    |....|....|....|....|....|                         
    0    1    2    3    4    5                          
    */
    north = 8499.9;
    east=  9999.9;
    top =  999.9;
        
    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);    
    printf("Rast3d_location2coord col %i row %i depth %i\n", col, row, depth);
    if(region.cols - 1 != col || 0 != row || region.depths - 1 != depth) {
        G_message("Error in Rast3d_location2coord");
        sum++;
    }
    
    G_message("Test the lower left corner, coordinates must be col = 0 row = 14 depth = 0");
    
    north = 1000.0;
    east= 5000.0;
    top = 0.0;
        
    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);    
    printf("Rast3d_location2coord col %i row %i depth %i\n", col, row, depth);
    if(0 != col || 14 != row || 0 != depth) {
        G_message("Error in Rast3d_location2coord");
        sum++;
    }
    
    G_message("Test the center, coordinates must be col = 4 row = 7 depth = 2");
    
    
    north = 4750.0;
    east= 7499.9;
    top = 500.0;
        
    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);    
    printf("Rast3d_location2coord col %i row %i depth %i\n", col, row, depth);
    if((region.cols - 1)/2 != col || (region.rows - 1)/2 != row || (region.depths - 1)/2 != depth) {
        G_message("Error in Rast3d_location2coord");
        sum++;
    }
    
    G_message("Test the n=3000.1, e=7000.1 and t=800.1, coordinates must be col = 4 row = 10 depth = 4");
    
    north = 3000.1;
    east= 7000.1;
    top = 800.1;
        
    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);    
    printf("Rast3d_location2coord col %i row %i depth %i\n", col, row, depth);
    if(4 != col || map->region.rows - 5 != row || 4 != depth) {
        G_message("Error in Rast3d_location2coord");
        sum++;
    }
    
    G_message("Test the n=2999.9, e=6999.9 and t=799.9, coordinates must be col = 3 row = 11 depth = 3");
    
    north = 2999.9;
    east= 6999.9;
    top = 799.9;
        
    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);    
    printf("Rast3d_location2coord col %i row %i depth %i\n", col, row, depth);
    if(3 != col || map->region.rows - 4 != row || 3 != depth) {
        G_message("Error in Rast3d_location2coord");
        sum++;
    }
    
    Rast3d_close(map);
    
    G_remove("grid3", "test_coordinate_transform");
    
    return sum;
}

/* *************************************************************** */

int test_region(void)
{
    int sum = 0;
    RASTER3D_Region region, new_region;
    
    Rast3d_get_window(&region);
    region.bottom = 0.0;
    region.top = 1000;
    region.south = 10000;
    region.north = 20000;
    region.west = 5000;
    region.east = 10000;
    region.rows = 20;
    region.cols = 10;
    region.depths = 5;
    region.ew_res = 0;
    region.ns_res = 0;
    region.tb_res = 0;
        
    /* Test region adjustment */
    Rast3d_adjust_region(&region);
    
    if(region.ew_res != 500) {
        G_message("Error in Rast3d_adjust_region: region.ew_res != 500");
        sum++;
    }
    
    if(region.ns_res != 500) {
        G_message("Error in Rast3d_adjust_region: region.ew_res != 500");
        sum++;
    }
    
    if(region.tb_res != 200) {
        G_message("Error in Rast3d_adjust_region: region.ew_res != 500");
        sum++;
    }
    
    /* Test the region copy */
    Rast3d_region_copy(&new_region, &region);
        
    if(region.bottom != new_region.bottom) {
        G_message("Error in Rast3d_region_copy: region.bottom != new_region.bottom");
        sum++;
    }
    
    if(region.cols != new_region.cols) {
        G_message("Error in Rast3d_region_copy: region.cols != new_region.cols");
        sum++;
    }
    
    if(region.depths != new_region.depths) {
        G_message("Error in Rast3d_region_copy: region.depths != new_region.depths");
        sum++;
    }
    
    if(region.east != new_region.east) {
        G_message("Error in Rast3d_region_copy: region.east != new_region.east");
        sum++;
    }
    
    if(region.ew_res != new_region.ew_res) {
        G_message("Error in Rast3d_region_copy: region.ew_res != new_region.ew_res");
        sum++;
    }
    
    if(region.north != new_region.north) {
        G_message("Error in Rast3d_region_copy: region.north != new_region.north");
        sum++;
    }
    
    if(region.ns_res != new_region.ns_res) {
        G_message("Error in Rast3d_region_copy: region.ns_res != new_region.ns_res");
        sum++;
    }
    
    if(region.proj != new_region.proj) {
        G_message("Error in Rast3d_region_copy: region.proj != new_region.proj");
        sum++;
    }
    
    if(region.rows != new_region.rows) {
        G_message("Error in Rast3d_region_copy: region.rows != new_region.rows");
        sum++;
    }
    
    if(region.south != new_region.south) {
        G_message("Error in Rast3d_region_copy: region.south != new_region.south");
        sum++;
    }
    
    if(region.tb_res != new_region.tb_res) {
        G_message("Error in Rast3d_region_copy: region.tb_res != new_region.tb_res");
        sum++;
    }
    
    if(region.top != new_region.top) {
        G_message("Error in Rast3d_region_copy: region.top != new_region.top");
        sum++;
    }
    
    if(region.west != new_region.west) {
        G_message("Error in Rast3d_region_copy: region.west != new_region.west");
        sum++;
    }
    
    if(region.zone != new_region.zone) {
        G_message("Error in Rast3d_region_copy: region.zone != new_region.zone");
        sum++;
    }    
    
    return sum;
}
