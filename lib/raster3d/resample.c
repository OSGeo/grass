#include <stdio.h>
#include <grass/gis.h>
#include "raster3d_intern.h"

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * The default resampling function which uses nearest
 * neighbor resampling. This method converts the window coordinates
 * x, y, and z into region coordinates and returned the nearest neighbor.
 *
 *  \param map
 *  \param col
 *  \param row
 *  \param depth
 *  \param value
 *  \param type
 *  \return void
 */

void
Rast3d_nearest_neighbor(RASTER3D_Map * map, int x, int y, int z, void *value,
		    int type)
{
    double north, east, top;
    int row, col, depth;

    /* convert (x, y, z) window coordinates into (north, east, top) */
    Rast3d_coord2location(&(map->window), (double)x + 0.5, (double)y + 0.5,
    		(double)z + 0.5, &north, &east, &top);

    /* convert (north, east, top) into map region coordinates (row, col, depth) */
    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);
    
    /* Get the value from the map in map-region resolution */
    Rast3d_get_value_region(map, col, row, depth, value, type);
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the resampling function to be used by
 * Rast3d_get_value () (cf.{g3d:G3d.getValue}). This function is defined
 * as follows:
 *
 *  \return void
 */

void Rast3d_set_resampling_fun(RASTER3D_Map * map, void (*resampleFun) ())
{
    map->resampleFun = resampleFun;
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * 
 * Returns in <em>resampleFun</em> a pointer to the resampling function used by
 * <em>map</em>.
 *
 *  \return void
 */

void Rast3d_get_resampling_fun(RASTER3D_Map * map, void (**resampleFun) ())
{
    *resampleFun = map->resampleFun;
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns
 * in <em>nnFunPtr</em> a pointer to Rast3d_nearest_neighbor () (cf.{g3d:G3d.nearestNeighbor}).
 *
 *  \return void
 */

void Rast3d_get_nearest_neighbor_fun_ptr(void (**nnFunPtr) ())
{
    *nnFunPtr = Rast3d_nearest_neighbor;
}
