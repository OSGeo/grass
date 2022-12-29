#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

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
Rast3d_tile_index2tile(RASTER3D_Map * map, int tileIndex, int *xTile, int *yTile,
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

int Rast3d_tile2tile_index(RASTER3D_Map * map, int xTile, int yTile, int zTile)
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
Rast3d_tile_coord_origin(RASTER3D_Map * map, int xTile, int yTile, int zTile, int *x,
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

void Rast3d_tile_index_origin(RASTER3D_Map * map, int tileIndex, int *x, int *y, int *z)
{
    int xTile, yTile, zTile;

    Rast3d_tile_index2tile(map, tileIndex, &xTile, &yTile, &zTile);
    Rast3d_tile_coord_origin(map, xTile, yTile, zTile, x, y, z);
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
Rast3d_coord2tile_coord(RASTER3D_Map * map, int x, int y, int z, int *xTile,
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
Rast3d_coord2tile_index(RASTER3D_Map * map, int x, int y, int z, int *tileIndex,
		    int *offset)
{
    int xTile, yTile, zTile, xOffs, yOffs, zOffs;

    Rast3d_coord2tile_coord(map, x, y, z,
			&xTile, &yTile, &zTile, &xOffs, &yOffs, &zOffs);
    *tileIndex = Rast3d_tile2tile_index(map, xTile, yTile, zTile);
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

int Rast3d_coord_in_range(RASTER3D_Map * map, int x, int y, int z)
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

int Rast3d_tile_index_in_range(RASTER3D_Map * map, int tileIndex)
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

int Rast3d_tile_in_range(RASTER3D_Map * map, int x, int y, int z)
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
Rast3d_compute_clipped_tile_dimensions(RASTER3D_Map * map, int tileIndex, int *rows,
				 int *cols, int *depths, int *xRedundant,
				 int *yRedundant, int *zRedundant)
{
    int x, y, z;

    Rast3d_tile_index2tile(map, tileIndex, &x, &y, &z);

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
 * \brief Compute the optimal tile size.
 *
 * This function computes tile sizes with an optimal ratio between tile dimensions and
 * minimized border tile overlapping.
 * Large dimensions (in most cases x and y) will be reduced more often than small dimensions to
 * fit the maxSize criteria.
 *
 *  \param region The region of the map
 *  \param type The type of the map (FCELL_TYPE or DCELL_TYPE)
 *  \param tileX Pointer of the tile size in x direction for result storage
 *  \param tileY Pointer of the tile size in y direction for result storage
 *  \param tileZ Pointer of the tile size in z direction for result storage
 *  \param maxSize The max size of the tile in kilo bytes
 *  \return void
 */

void
Rast3d_compute_optimal_tile_dimension(RASTER3D_Region *region, int type, int *tileX, int *tileY, int *tileZ, int maxSize)
{
   unsigned long size = 0;
   unsigned long x, y, z;
   unsigned long i = 0;
   unsigned long tileSize;
   unsigned long divx = 2;
   unsigned long divy = 2;
   unsigned long divz = 2;

   if(type == FCELL_TYPE)
      size = sizeof(FCELL);

   if(type == DCELL_TYPE)
      size = sizeof(DCELL);
   
   x = region->cols;
   y = region->rows;
   z = region->depths;

   while(1) {
       tileSize = size * x * y * z;

       G_debug(2, "Rast3d_compute_optimal_tile_dimension: tilesize %li x %li y %li z %li\n", tileSize, x, y, z);

       if(tileSize <= maxSize * 1024)
          break;

       /* Compute weighted tile sizes. Take care that the tile size is computed based on
          the dimension ratio and reduce the border tile overlapping. 
          In case one dimension is much larger than the other, reduce 
          the large dimension by a factor till the maxSize is reached or till the 
          the other dimensions are only by factor 2 smaller.*/
       if((y / x) <= 2 && (z / x) <= 2) {
           if(region->cols % divx != 0)
               x = region->cols / divx + 1;
           else
               x = region->cols / divx;
           divx += 1;
       }
       if((x / y) <= 2 && (z / y) <= 2) {
           if(region->rows % divy != 0)
               y = region->rows / divy + 1;
           else
               y = region->rows / divy;
           divy += 1;
       }
       if((x / z) <= 2 && (y / z) <= 2) {
           if(region->depths % divz != 0)
               z = region->depths / divz + 1;
           else
               z = region->depths / divz;
           divz += 1;
       }

       /* Avoid infinite loop */
       i++;
       if(i > 10000)
         break;
   }

   *tileX = (int)x;
   *tileY = (int)y;
   *tileZ = (int)z;
}


