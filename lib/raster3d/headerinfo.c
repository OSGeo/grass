#include <grass/raster3d.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns the size of the region of <em>map</em> in cells.
 *
 *  \param map
 *  \param rows
 *  \param cols
 *  \param depths
 *  \return void
 */

void Rast3d_getCoordsMap(RASTER3D_Map * map, int *rows, int *cols, int *depths)
{
    *rows = map->region.rows;
    *cols = map->region.cols;
    *depths = map->region.depths;
}

/*---------------------------------------------------------------------------*/

void Rast3d_getCoordsMapWindow(RASTER3D_Map * map, int *rows, int *cols, int *depths)
{
    *rows = map->window.rows;
    *cols = map->window.cols;
    *depths = map->window.depths;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns the dimensions of the tile-cube used to tile the region of <em>map</em>.
 * These numbers include partial tiles.
 *
 *  \param map
 *  \param nx
 *  \param ny
 *  \param nz
 *  \return void
 */

void Rast3d_getNofTilesMap(RASTER3D_Map * map, int *nx, int *ny, int *nz)
{
    *nx = map->nx;
    *ny = map->ny;
    *nz = map->nz;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns the size of the region.
 *
 *  \param map
 *  \param north
 *  \param south
 *  \param east
 *  \param west
 *  \param top
 *  \param bottom
 *  \return void
 */

void
Rast3d_getRegionMap(RASTER3D_Map * map, double *north, double *south, double *east,
		 double *west, double *top, double *bottom)
{
    *north = map->region.north;
    *south = map->region.south;
    *east = map->region.east;
    *west = map->region.west;
    *top = map->region.top;
    *bottom = map->region.bottom;
}

/*---------------------------------------------------------------------------*/

void
Rast3d_getWindowMap(RASTER3D_Map * map, double *north, double *south, double *east,
		 double *west, double *top, double *bottom)
{
    *north = map->window.north;
    *south = map->window.south;
    *east = map->window.east;
    *west = map->window.west;
    *top = map->window.top;
    *bottom = map->window.bottom;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns in <em>region</em> the region of <em>map</em>.
 *
 *  \param map
 *  \param region
 *  \return void
 */

void Rast3d_getRegionStructMap(RASTER3D_Map * map, RASTER3D_Region * region)
{
    Rast3d_regionCopy(region, &(map->region));
}

/*---------------------------------------------------------------------------*/

void Rast3d_getWindowStructMap(RASTER3D_Map * map, RASTER3D_Region * window)
{
    Rast3d_regionCopy(window, &(map->window));
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns the tile dimensions used for <em>map</em>.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return void
 */

void Rast3d_getTileDimensionsMap(RASTER3D_Map * map, int *x, int *y, int *z)
{
    *x = map->tileX;
    *y = map->tileY;
    *z = map->tileZ;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns the type in which tiles of <em>map</em> are stored in memory.
 *
 *  \param map
 *  \return int
 */

int Rast3d_tileTypeMap(RASTER3D_Map * map)
{
    return map->typeIntern;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns the type with which tiles of <em>map</em> are stored on file.
 *
 *  \param map
 *  \return int
 */

int Rast3d_fileTypeMap(RASTER3D_Map * map)
{
    return map->type;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns the precision used to store <em>map</em>.
 *
 *  \param map
 *  \return int
 */

int Rast3d_tilePrecisionMap(RASTER3D_Map * map)
{
    return map->precision;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns 1 if <em>map</em> uses cache, returns 0 otherwise.
 *
 *  \param map
 *  \return int
 */

int Rast3d_tileUseCacheMap(RASTER3D_Map * map)
{
    return map->useCache;
}



/*!
 * \brief 
 *
 * Prints the header information of <em>map</em>.
 *
 *  \param map
 *  \return void
 */

void Rast3d_printHeader(RASTER3D_Map * map)
{
    double rangeMin, rangeMax;

    printf("File %s open for %sing:\n", map->fileName,
	   (map->operation == RASTER3D_WRITE_DATA ? "writ" :
	    (map->operation == RASTER3D_READ_DATA ? "read" : "unknown")));
    printf("  Fd = %d, Unit %s, Type: %s, ", map->data_fd,
	   map->unit,
	   (map->type == FCELL_TYPE ? "float" :
	    (map->type == DCELL_TYPE ? "double" : "unknown")));
    printf("Type intern: %s\n",
	   (map->typeIntern == FCELL_TYPE ? "float" :
	    (map->typeIntern == DCELL_TYPE ? "double" : "unknown")));
    if (map->compression == RASTER3D_NO_COMPRESSION)
	printf("  Compression: none\n");
    else {
	printf("  Compression:%s%s Precision: %s",
	       (map->useLzw ? " lzw," : ""), (map->useRle ? " rle," : ""),
	       (map->precision == -1 ? "all bits used\n" : "using"));
	if (map->precision != -1)
	    printf(" %d bits\n", map->precision);
    }

    if (!map->useCache)
	printf("  Cache: none\n");
    else {
	printf("  Cache: used%s\n",
	       (map->operation == RASTER3D_WRITE_DATA ? ", File Cache used" : ""));
    }

    Rast3d_range_min_max(map, &rangeMin, &rangeMax);

    printf("  Region: (%f %f) (%f %f) (%f %f)\n",
	   map->region.south, map->region.north, map->region.west,
	   map->region.east, map->region.bottom, map->region.top);
    printf("          (%d %d %d)\n", map->region.rows, map->region.cols,
	   map->region.depths);
    printf("  Tile size (%d %d %d)\n", map->tileX, map->tileY, map->tileZ);
    printf("  Range (");
    if (Rast3d_isNullValueNum(&rangeMin, DCELL_TYPE))
	printf("NULL, ");
    else
	printf("%f, ", (double)rangeMin);
    if (Rast3d_isNullValueNum(&rangeMax, DCELL_TYPE))
	printf("NULL)\n");
    else
	printf("%f)\n", (double)rangeMax);
    fflush(stdout);
}
