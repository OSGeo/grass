#include <grass/raster.h>
#include "raster3d_intern.h"



/*!
 * \brief 
 *
 *  Is equivalent to Rast3d_put_value (map, x, y, z, &value, FCELL_TYPE).
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param value
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */


int Rast3d_put_float(RASTER3D_Map * map, int x, int y, int z, float value)
{
    int tileIndex, offs;
    float *tile;

    if (map->typeIntern == DCELL_TYPE)
    	return (Rast3d_put_double(map, x, y, z, (double)value));

    Rast3d_coord2tile_index(map, x, y, z, &tileIndex, &offs);
    tile = (float *)Rast3d_get_tile_ptr(map, tileIndex);
    if (tile == NULL) {
	Rast3d_error("Rast3d_put_float: error in Rast3d_get_tile_ptr");
	return 0;
    }

    tile[offs] = value;
    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to Rast3d_put_value (map, x, y, z, &value, DCELL_TYPE).
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param value
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int Rast3d_put_double(RASTER3D_Map * map, int x, int y, int z, double value)
{
    int tileIndex, offs;
    double *tile;

    if (map->typeIntern == FCELL_TYPE)
    	return (Rast3d_put_float(map, x, y, z, (float)value));

    Rast3d_coord2tile_index(map, x, y, z, &tileIndex, &offs);
    tile = (double *)Rast3d_get_tile_ptr(map, tileIndex);
    if (tile == NULL) {
	Rast3d_error("Rast3d_put_double: error in Rast3d_get_tile_ptr");
	return 0;
    }

    tile[offs] = value;
    return 1;
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * After converting <em>*value</em> of <em>type</em> into the type specified
 * at the initialization time (i.e. <em>typeIntern</em>) this function writes the
 * value into the tile buffer corresponding to cell-coordinate <em>(x, y, z)</em>.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param value
 *  \param type
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int
Rast3d_put_value(RASTER3D_Map * map, int x, int y, int z, const void *value, int type)
{
    if (type == FCELL_TYPE)
    	return (Rast3d_put_float(map, x, y, z, *((float *)value)));

	return (Rast3d_put_double(map, x, y, z, *((double *)value)));

}
