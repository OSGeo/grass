#include <grass/raster.h>
#include "G3d_intern.h"

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

void G3d_getValue(G3D_Map * map, int x, int y, int z, void *value, int type)
{
    /* get the resampled value */
    map->resampleFun(map, x, y, z, value, type);
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent to
 * <tt>G3d_getValue (map, x, y, z, &value, FCELL_TYPE);</tt> return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return float
 */

float G3d_getFloat(G3D_Map * map, int x, int y, int z)
{
    float value;

    G3d_getValue(map, x, y, z, &value, FCELL_TYPE);
    return value;
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent
 * to <tt>G3d_getValue (map, x, y, z, &value, DCELL_TYPE);</tt> return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return double
 */

double G3d_getDouble(G3D_Map * map, int x, int y, int z)
{
    double value;

    G3d_getValue(map, x, y, z, &value, DCELL_TYPE);
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
G3d_getWindowValue(G3D_Map * map, double north, double east, double top,
		   void *value, int type)
{
    int col, row, depth;

    G3d_location2coord(&(map->window), north, east, top, &col, &row, &depth);

    /* if (row, col, depth) outside window return NULL value */
    if ((row < 0) || (row >= map->window.rows) ||
	(col < 0) || (col >= map->window.cols) ||
	(depth < 0) || (depth >= map->window.depths)) {
	G3d_setNullValue(value, 1, type);
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
G3d_getRegionValue(G3D_Map * map, double north, double east, double top,
		   void *value, int type)
{
    int row, col, depth;

    G3d_location2coord(&(map->region), north, east, top, &col, &row, &depth);

    /* if (row, col, depth) outside region return NULL value */
    if ((row < 0) || (row >= map->region.rows) ||
	(col < 0) || (col >= map->region.cols) ||
	(depth < 0) || (depth >= map->region.depths)) {
	G3d_setNullValue(value, 1, type);
	return;
    }

    /* Get the value from the map in map-region resolution */
	G3d_getValueRegion(map, col, row, depth, value, type);
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * Is equivalent to <tt>G3d_getValueRegion (map, x, y, z, &value, FCELL_TYPE);</tt>
 * return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return float
 */

float G3d_getFloatRegion(G3D_Map * map, int x, int y, int z)
{
    int tileIndex, offs;
    float *tile;

    if (map->typeIntern == DCELL_TYPE)
	return (float)G3d_getDoubleRegion(map, x, y, z);

    G3d_coord2tileIndex(map, x, y, z, &tileIndex, &offs);
    tile = (float *)G3d_getTilePtr(map, tileIndex);

    if (tile == NULL)
	G3d_fatalError("G3d_getFloatRegion: error in G3d_getTilePtr");

    return tile[offs];
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Is equivalent to <tt>G3d_getValueRegion (map, x, y, z, &value,
 * DCELL_TYPE);</tt> return value.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return double
 */

double G3d_getDoubleRegion(G3D_Map * map, int x, int y, int z)
{
    int tileIndex, offs;
    double *tile;

    if (map->typeIntern == FCELL_TYPE)
	return (double)G3d_getFloatRegion(map, x, y, z);

    G3d_coord2tileIndex(map, x, y, z, &tileIndex, &offs);
    tile = (double *)G3d_getTilePtr(map, tileIndex);

    if (tile == NULL)
	G3d_fatalError("G3d_getDoubleRegion: error in G3d_getTilePtr");

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
G3d_getValueRegion(G3D_Map * map, int x, int y, int z, void *value, int type)
{
    if (type == FCELL_TYPE) {
	*((float *)value) = G3d_getFloatRegion(map, x, y, z);
	return;
    }

    *((double *)value) = G3d_getDoubleRegion(map, x, y, z);
}
