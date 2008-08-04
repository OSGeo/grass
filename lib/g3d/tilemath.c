#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Converts index <em>tileIndex</em> into tile-coordinates
 * <em>(xTile, yTile, zTile)</em>.
 *
 *  \param map
 *  \param tileIndex
 *  \param xTile
 *  \param yTile
 *  \param zTile
 *  \return void
 */

void
G3d_tileIndex2tile(G3D_Map * map, int tileIndex, int *xTile, int *yTile,
		   int *zTile)
{
    int tileIndex2d;

    *zTile = tileIndex / map->nxy;
    tileIndex2d = tileIndex % map->nxy;
    *yTile = tileIndex2d / map->nx;
    *xTile = tileIndex2d % map->nx;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns tile-index corresponding to tile-coordinates <em>(xTile,
 * yTile, zTile)</em>.
 *
 *  \param map
 *  \param xTile
 *  \param yTile
 *  \param zTile
 *  \return int
 */

int G3d_tile2tileIndex(G3D_Map * map, int xTile, int yTile, int zTile)
{
    return map->nxy * zTile + map->nx * yTile + xTile;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Computes the cell-coordinates <em>(x, y, z)</em>
 * which correspond to the origin of the tile with tile-coordinates <em>(xTile,
 * yTile, zTile)</em>.
 *
 *  \param map
 *  \param xTile
 *  \param yTile
 *  \param zTile
 *  \param x
 *  \param y
 *  \param z
 *  \return void
 */

void
G3d_tileCoordOrigin(G3D_Map * map, int xTile, int yTile, int zTile, int *x,
		    int *y, int *z)
{
    *x = map->tileX * xTile;
    *y = map->tileY * yTile;
    *z = map->tileZ * zTile;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Computes the cell-coordinates <em>(x, y, z)</em> which correspond to
 * the origin of the tile with <em>tileIndex</em>.
 *
 *  \param map
 *  \param tileIndex
 *  \param x
 *  \param y
 *  \param z
 *  \return void
 */

void G3d_tileIndexOrigin(G3D_Map * map, int tileIndex, int *x, int *y, int *z)
{
    int xTile, yTile, zTile;

    G3d_tileIndex2tile(map, tileIndex, &xTile, &yTile, &zTile);
    G3d_tileCoordOrigin(map, xTile, yTile, zTile, x, y, z);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Converts
 * cell-coordinates <em>(x, y, z)</em> into tile-coordinates <em>(xTile, yTile,
 * zTile)</em> and the coordinate of the cell <em>(xOffs, yOffs, zOffs)</em> within
 * the tile.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param xTile
 *  \param yTile
 *  \param zTile
 *  \param xOffs
 *  \param yOffs
 *  \param zOffs
 *  \return void
 */

void
G3d_coord2tileCoord(G3D_Map * map, int x, int y, int z, int *xTile,
		    int *yTile, int *zTile, int *xOffs, int *yOffs,
		    int *zOffs)
{
    *xTile = x / map->tileX;
    *xOffs = x % map->tileX;
    *yTile = y / map->tileY;
    *yOffs = y % map->tileY;
    *zTile = z / map->tileZ;
    *zOffs = z % map->tileZ;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Converts cell-coordinates <em>(x, y, z)</em> into
 * <em>tileIndex</em> and the <em>offset</em> of the cell within the tile.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \param tileIndex
 *  \param offset
 *  \return void
 */

void
G3d_coord2tileIndex(G3D_Map * map, int x, int y, int z, int *tileIndex,
		    int *offset)
{
    int xTile, yTile, zTile, xOffs, yOffs, zOffs;

    G3d_coord2tileCoord(map, x, y, z,
			&xTile, &yTile, &zTile, &xOffs, &yOffs, &zOffs);
    *tileIndex = G3d_tile2tileIndex(map, xTile, yTile, zTile);
    *offset = zOffs * map->tileXY + yOffs * map->tileX + xOffs;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if
 * cell-coordinate <em>(x, y, z)</em> is a coordinate inside the region. Returns 0
 * otherwise.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return int
 */

int G3d_coordInRange(G3D_Map * map, int x, int y, int z)
{
    return (x >= 0) && (x < map->region.cols) && (y >= 0) &&
	(y < map->region.rows) && (z >= 0) && (z < map->region.depths);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if <em>tileIndex</em> is a valid index for <em>map</em>.
 *  Returns 0 otherwise.
 *
 *  \param map
 *  \param tileIndex
 *  \return int
 */

int G3d_tileIndexInRange(G3D_Map * map, int tileIndex)
{
    return (tileIndex < map->nTiles) && (tileIndex >= 0);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if
 * tile-coordinate <em>(x, y, z)</em> is a coordinate inside tile cube. Returns 0
 * otherwise.
 *
 *  \param map
 *  \param x
 *  \param y
 *  \param z
 *  \return int
 */

int G3d_tileInRange(G3D_Map * map, int x, int y, int z)
{
    return (x >= 0) && (x < map->nx) && (y >= 0) && (y < map->ny) &&
	(z >= 0) && (z < map->nz);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Computes the dimensions of the tile when clipped to fit the
 * region of <em>map</em>. The clipped dimensions are returned in <em>rows</em>, 
 * <em>cols</em>, <em>depths</em>.  The complement is returned in <em>xRedundant</em>,
 *  <em>yRedundant</em>, and <em>zRedundant</em>. This function returns the number of
 * cells in the clipped tile.
 *
 *  \param map
 *  \param tileIndex
 *  \param rows
 *  \param cols
 *  \param depths
 *  \param xRedundant
 *  \param yRedundant
 *  \param zRedundant
 *  \return int
 */

int
G3d_computeClippedTileDimensions(G3D_Map * map, int tileIndex, int *rows,
				 int *cols, int *depths, int *xRedundant,
				 int *yRedundant, int *zRedundant)
{
    int x, y, z;

    G3d_tileIndex2tile(map, tileIndex, &x, &y, &z);

    if ((x != map->clipX) && (y != map->clipY) && (z != map->clipZ)) {
	return map->tileSize;
    }

    if (x != map->clipX) {
	*cols = map->tileX;
	*xRedundant = 0;
    }
    else {
	*cols = (map->region.cols - 1) % map->tileX + 1;
	*xRedundant = map->tileX - *cols;
    }
    if (y != map->clipY) {
	*rows = map->tileY;
	*yRedundant = 0;
    }
    else {
	*rows = (map->region.rows - 1) % map->tileY + 1;
	*yRedundant = map->tileY - *rows;
    }
    if (z != map->clipZ) {
	*depths = map->tileZ;
	*zRedundant = 0;
    }
    else {
	*depths = (map->region.depths - 1) % map->tileZ + 1;
	*zRedundant = map->tileZ - *depths;
    }

    /* printf ("%d (%d %d %d): (%d %d) (%d %d) (%d %d), %d\n", */
    /*      tileIndex, x, y, z, *rows, *xRedundant, *cols, *yRedundant,  */
    /*      *depths, *zRedundant, *depths * *cols * *rows); */

    return *depths * *cols * *rows;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if region-coordinates <em>(north, west, bottom)</em> are
 * inside the region of <em>map</em>. Returns 0 otherwise.
 *
 *  \param map
 *  \param north
 *  \param west
 *  \param bottom
 *  \return int
 */

int G3d_isValidLocation(G3D_Map * map, double north, double east, double top)
{
    return ((north >= map->region.south) && (north <= map->region.north) &&
	    (east >= map->region.west) && (east <= map->region.east) &&
	    (((top >= map->region.bottom) && (top <= map->region.top)) ||
	     ((top <= map->region.bottom) && (top >= map->region.top))));
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Converts region-coordinates <em>(north, west,
 * bottom)</em> into cell-coordinates <em>(x, y, z)</em>.
 *
 *  \param map
 *  \param north
 *  \param west
 *  \param bottom
 *  \param x
 *  \param y
 *  \param z
 *  \return void
 */

void
G3d_location2coord(G3D_Map * map, double north, double east, double top,
		   int *x, int *y, int *z)
{
    if (!G3d_isValidLocation(map, north, east, top))
	G3d_fatalError("location2coord: location not in region");

    *y = (north - map->region.south) / (map->region.north -
					map->region.south) *
	(map->region.rows - 1);
    *x = (east - map->region.west) / (map->region.east -
				      map->region.west) * (map->region.cols -
							   1);
    *z = (top - map->region.bottom) / (map->region.top -
				       map->region.bottom) *
	(map->region.depths - 1);
}
