#include <grass/raster.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Returns in <em>*value</em> the resampled cell-value of the cell with
 * window-coordinate <em>(x, y, z)</em>.  The value returned is of <em>type</em>.
 * This function invokes a fatal error if an error occurs.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param value
 *  \param type
 *  \return void
 */

void Rast3d_get_value(RASTER3D_Map * map, int x, int y, int z, void *value, int type)
{
    /* get the resampled value */
    map->resampleFun(map, x, y, z, value, type);
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent to
 * <tt>Rast3d_get_value (map, x, y, z, &value, FCELL_TYPE);</tt> return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return float
 */

float Rast3d_get_float(RASTER3D_Map * map, int x, int y, int z)
{
    float value;

    Rast3d_get_value(map, x, y, z, &value, FCELL_TYPE);
    return value;
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent
 * to <tt>Rast3d_get_value (map, x, y, z, &value, DCELL_TYPE);</tt> return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return double
 */

double Rast3d_get_double(RASTER3D_Map * map, int x, int y, int z)
{
    double value;

    Rast3d_get_value(map, x, y, z, &value, DCELL_TYPE);
    return value;
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Returns in <em>value</em> the value of the <em>map</em> which corresponds to 
 * window coordinates <em>(north, east, top)</em>.  The
 * value is resampled using the resampling function specified for <em>map</em>. The
 * <em>value</em> is of <em>type</em>.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \param value
 *  \param type
 *  \return void
 */

void
Rast3d_get_window_value(RASTER3D_Map * map, double north, double east, double top,
		   void *value, int type)
{
    int col, row, depth;

    Rast3d_location2coord(&(map->window), north, east, top, &col, &row, &depth);

    /* if (row, col, depth) outside window return NULL value */
    if ((row < 0) || (row >= map->window.rows) ||
	(col < 0) || (col >= map->window.cols) ||
	(depth < 0) || (depth >= map->window.depths)) {
	Rast3d_set_null_value(value, 1, type);
	return;
    }

    /* Get the value from the map in map-region resolution */
	map->resampleFun(map, col, row, depth, value, type);
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Returns in <em>value</em> the value of the <em>map</em> which corresponds to 
 * region coordinates <em>(north, east, top)</em>.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \param value
 *  \param type
 *  \return void
 */

void
Rast3d_get_region_value(RASTER3D_Map * map, double north, double east, double top,
		   void *value, int type)
{
    int row, col, depth;

    Rast3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);

    /* if (row, col, depth) outside region return NULL value */
    if ((row < 0) || (row >= map->region.rows) ||
	(col < 0) || (col >= map->region.cols) ||
	(depth < 0) || (depth >= map->region.depths)) {
	Rast3d_set_null_value(value, 1, type);
	return;
    }

    /* Get the value from the map in map-region resolution */
	Rast3d_get_value_region(map, col, row, depth, value, type);
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent to <tt>Rast3d_get_value_region (map, x, y, z, &value, FCELL_TYPE);</tt>
 * return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return float
 */

float Rast3d_get_float_region(RASTER3D_Map * map, int x, int y, int z)
{
    int tileIndex, offs;
    float *tile;
    float value;

    if (map->typeIntern == DCELL_TYPE)
	return (float)Rast3d_get_double_region(map, x, y, z);

    /* In case of region coordinates out of bounds, return the Null value */
    if(x < 0 || y < 0 || z < 0 || x >= map->region.cols ||
       y >= map->region.rows || z >= map->region.depths) {
         Rast3d_set_null_value(&value, 1, FCELL_TYPE);
         return value;
    }   

    Rast3d_coord2tile_index(map, x, y, z, &tileIndex, &offs);
    tile = (float *)Rast3d_get_tile_ptr(map, tileIndex);

    if (tile == NULL)
	Rast3d_fatal_error("Rast3d_get_float_region: error in Rast3d_get_tile_ptr." 
                           "Region coordinates x %i y %i z %i  tile index %i offset %i",
                           x, y, z, tileIndex, offs);

    return tile[offs];
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent to <tt>Rast3d_get_value_region (map, x, y, z, &value,
 * DCELL_TYPE);</tt> return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return double
 */

double Rast3d_get_double_region(RASTER3D_Map * map, int x, int y, int z)
{
    int tileIndex, offs;
    double *tile;
    double value;

    if (map->typeIntern == FCELL_TYPE)
	return (double)Rast3d_get_float_region(map, x, y, z);

    /* In case of region coordinates out of bounds, return the Null value */
    if(x < 0 || y < 0 || z < 0 || x >= map->region.cols ||
       y >= map->region.rows || z >= map->region.depths) {
         Rast3d_set_null_value(&value, 1, DCELL_TYPE);
         return value;
    } 

    Rast3d_coord2tile_index(map, x, y, z, &tileIndex, &offs);
    tile = (double *)Rast3d_get_tile_ptr(map, tileIndex);

    if (tile == NULL)
	Rast3d_fatal_error("Rast3d_get_double_region: error in Rast3d_get_tile_ptr." 
                           "Region coordinates x %i y %i z %i  tile index %i offset %i",
                           x, y, z, tileIndex, offs);

    return tile[offs];
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Returns in <em>*value</em> the cell-value of the cell with
 * region-coordinate <em>(x, y, z)</em>.  The value returned is of <em>type</em>.
 * Here <em>region</em> means the coordinate in the cube of data in the file, i.e.
 * ignoring geographic coordinates.
 * In case the region coordinates are out of bounds, the Null value will be returned.
 * This function invokes a fatal error if an error occurs.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param value
 *  \param type
 *  \return void
 */

void
Rast3d_get_value_region(RASTER3D_Map * map, int x, int y, int z, void *value, int type)
{
    if (type == FCELL_TYPE) {
	*((float *)value) = Rast3d_get_float_region(map, x, y, z);
	return;
    }

    *((double *)value) = Rast3d_get_double_region(map, x, y, z);
}
