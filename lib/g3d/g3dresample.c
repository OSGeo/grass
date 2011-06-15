#include <stdio.h>
#include <grass/gis.h>
#include "G3d_intern.h"

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
G3d_nearestNeighbor(G3D_Map * map, int x, int y, int z, void *value,
		    int type)
{

    double north, east, top;
    int row, col, depth;

    /* convert (x, y, z) into (north, east, top) */

	north = ((double)y + 0.5) / (double)map->window.rows *
	(map->window.north - map->window.south) + map->window.south;
    east = ((double)x + 0.5) / (double)map->window.cols *
	(map->window.east - map->window.west) + map->window.west;
    top = ((double)z + 0.5) / (double)map->window.depths *
	(map->window.top - map->window.bottom) + map->window.bottom;

    /* convert (north, east, top) into (row, col, depth) */

    G3d_location2coord(map, north, east, top, &col, &row, &depth);

    /* if (row, col, depth) outside map region return NULL value */
    if ((row < 0) || (row >= map->region.rows) ||
	(col < 0) || (col >= map->region.cols) ||
	(depth < 0) || (depth >= map->region.depths)) {
	G3d_setNullValue(value, 1, type);
	return;
    }
    
    /* Get the value from the map in map-region resolution */
	G3d_getValueRegion(map, col, row, depth, value, type);
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the resampling function to be used by
 * G3d_getValue () (cf.{g3d:G3d.getValue}). This function is defined
 * as follows:
 *
 *  \return void
 */

void G3d_setResamplingFun(G3D_Map * map, void (*resampleFun) ())
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

void G3d_getResamplingFun(G3D_Map * map, void (**resampleFun) ())
{
    *resampleFun = map->resampleFun;
}

/*--------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns
 * in <em>nnFunPtr</em> a pointer to G3d_nearestNeighbor () (cf.{g3d:G3d.nearestNeighbor}).
 *
 *  \return void
 */

void G3d_getNearestNeighborFunPtr(void (**nnFunPtr) ())
{
    *nnFunPtr = G3d_nearestNeighbor;
}
