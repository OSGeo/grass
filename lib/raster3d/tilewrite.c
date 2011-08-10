#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include <grass/raster.h>
#include "raster3d_intern.h"


/*---------------------------------------------------------------------------*/

static int
G3d_tile2xdrTile(RASTER3D_Map * map, const void *tile, int rows, int cols,
		 int depths, int xRedundant, int yRedundant, int zRedundant,
		 int nofNum, int type)
{
    int y, z;

    if (!G3d_initCopyToXdr(map, type)) {
	G3d_error("G3d_tile2xdrTile: error in G3d_initCopyToXdr");
	return 0;
    }


    if (nofNum == map->tileSize) {
	if (!G3d_copyToXdr(tile, map->tileSize)) {
	    G3d_error("G3d_tile2xdrTile: error in G3d_copyToXdr");
	    return 0;
	}
	return 1;
    }

    if (xRedundant) {
	for (z = 0; z < depths; z++) {
	    for (y = 0; y < rows; y++) {
		if (!G3d_copyToXdr(tile, cols)) {
		    G3d_error("G3d_tile2xdrTile: error in G3d_copyToXdr");
		    return 0;
		}
		tile = G_incr_void_ptr(tile, map->tileX * G3d_length(type));
	    }
	    if (yRedundant)
		tile =
		    G_incr_void_ptr(tile,
				    map->tileX * yRedundant *
				    G3d_length(type));
	}
	return 1;
    }

    if (yRedundant) {
	for (z = 0; z < depths; z++) {
	    if (!G3d_copyToXdr(tile, map->tileX * rows)) {
		G3d_error("G3d_tile2xdrTile: error in G3d_copyToXdr");
		return 0;
	    }
	    tile = G_incr_void_ptr(tile, map->tileXY * G3d_length(type));
	}
	return 1;
    }

    if (!G3d_copyToXdr(tile, map->tileXY * depths)) {
	G3d_error("G3d_tile2xdrTile: error in G3d_copyToXdr");
	return 0;
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_writeTileUncompressed(RASTER3D_Map * map, int nofNum)
{
    if (write(map->data_fd, xdr, map->numLengthExtern * nofNum) !=
	map->numLengthExtern * nofNum) {
	G3d_error("G3d_writeTileUncompressed: can't write file.");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_writeTileCompressed(RASTER3D_Map * map, int nofNum)
{
    if (!G_fpcompress_writeXdrNums(map->data_fd, xdr, nofNum, map->precision,
				   tmpCompress, map->type == FCELL_TYPE,
				   map->useRle, map->useLzw)) {
	G3d_error
	    ("G3d_writeTileCompressed: error in G_fpcompress_writeXdrNums");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

		       /* EXPORTED FUNCTIONS */

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * 
 * Writes tile with index <em>tileIndex</em> to the file corresponding to <em>map</em>. 
 * It is assumed that the cells in <em>tile</em> are of <em>type</em> which
 * must be one of FCELL_TYPE and DCELL_TYPE.  The actual type used to write the
 * tile depends on the type specified at the time when <em>map</em> is initialized.
 * A tile can only be written once. Subsequent attempts to write the same tile
 * are ignored.
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \param type
 *  \return 1 ... if successful,
 *          2 ... if write request was ignored,
 *          0 ... otherwise.
 */

int G3d_writeTile(RASTER3D_Map * map, int tileIndex, const void *tile, int type)
{
    int rows, cols, depths, xRedundant, yRedundant, zRedundant, nofNum;

    /* valid tileIndex ? */
    if ((tileIndex >= map->nTiles) || (tileIndex < 0))
	G3d_fatalError("G3d_writeTile: tileIndex out of range");

    /* already written ? */
    if (map->index[tileIndex] != -1)
	return 2;

    /* save the file position */
    map->index[tileIndex] = lseek(map->data_fd, (long)0, SEEK_END);
    if (map->index[tileIndex] == -1) {
	G3d_error("G3d_writeTile: can't position file");
	return 0;
    }

    nofNum = G3d_computeClippedTileDimensions(map, tileIndex,
					      &rows, &cols, &depths,
					      &xRedundant, &yRedundant,
					      &zRedundant);

    G3d_range_updateFromTile(map, tile, rows, cols, depths,
			     xRedundant, yRedundant, zRedundant, nofNum,
			     type);

    if (!G3d_tile2xdrTile(map, tile, rows, cols, depths,
			  xRedundant, yRedundant, zRedundant, nofNum, type)) {
	G3d_error("G3d_writeTileCompressed: error in G3d_tile2xdrTile");
	return 0;
    }

    if (map->compression == RASTER3D_NO_COMPRESSION) {
	if (!G3d_writeTileUncompressed(map, nofNum)) {
	    G3d_error("G3d_writeTile: error in G3d_writeTileUncompressed");
	    return 0;
	}
    }
    else if (!G3d_writeTileCompressed(map, nofNum)) {
	G3d_error("G3d_writeTile: error in G3d_writeTileCompressed");
	return 0;
    }

    /* compute the length */
    map->tileLength[tileIndex] = lseek(map->data_fd, (long)0, SEEK_END) -
	map->index[tileIndex];

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to <tt>G3d_writeTile (map, tileIndex, tile, FCELL_TYPE).</tt>
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \return int
 */

int G3d_writeTileFloat(RASTER3D_Map * map, int tileIndex, const void *tile)
{
    int status;

    if ((status = G3d_writeTile(map, tileIndex, tile, FCELL_TYPE)))
	return status;

    G3d_error("G3d_writeTileFloat: error in G3d_writeTile");
    return 0;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Is equivalent to <tt>G3d_writeTile (map, tileIndex, tile, DCELL_TYPE).</tt>
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \return int
 */

int G3d_writeTileDouble(RASTER3D_Map * map, int tileIndex, const void *tile)
{
    int status;

    if ((status = G3d_writeTile(map, tileIndex, tile, DCELL_TYPE)))
	return status;

    G3d_error("G3d_writeTileDouble: error in G3d_writeTile");
    return 0;
}

/*---------------------------------------------------------------------------*/

		      /* CACHE-MODE-ONLY FUNCTIONS */

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Writes the tile with
 * <em>tileIndex</em> to the file corresponding to <em>map</em> and removes the tile
 * from the cache (in non-cache mode the buffer provided by the map-structure is
 * written).
 * If this tile has already been written before the write request is ignored.
 * If the tile was never referred to before the invokation of G3d_flushTile, a
 * tile filled with NULL-values is written.
 *
 *  \param map
 *  \param tileIndex
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int G3d_flushTile(RASTER3D_Map * map, int tileIndex)
{
    const void *tile;

    tile = G3d_getTilePtr(map, tileIndex);
    if (tile == NULL) {
	G3d_error("G3d_flushTile: error in G3d_getTilePtr");
	return 0;
    }

    if (!G3d_writeTile(map, tileIndex, tile, map->typeIntern)) {
	G3d_error("G3d_flushTile: error in G3d_writeTile");
	return 0;
    }

    if (!G3d__removeTile(map, tileIndex)) {
	G3d_error("G3d_flushTile: error in G3d__removeTile");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)
#endif


/*!
 * \brief 
 *
 *  Writes the tiles with tile-coordinates
 * contained in the axis-parallel cube with vertices <em>(xMin, yMin, zMin)</em>
 * and <em>(xMax, yMax, zMax</em>).  Tiles which are not stored in the cache are
 * written as NULL-tiles.  Write attempts for tiles which have already been
 * written earlier are ignored.
 *
 *  \param map
 *  \param xMin
 *  \param yMin
 *  \param zMin
 *  \param xMax
 *  \param yMax
 *  \param zMax
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int
G3d_flushTileCube(RASTER3D_Map * map, int xMin, int yMin, int zMin, int xMax,
		  int yMax, int zMax)
{
    int x, y, z;

    if (!map->useCache)
	G3d_fatalError
	    ("G3d_flushTileCube: function invalid in non-cache mode");

    for (x = xMin; x <= xMax; x++)
	for (y = yMin; y <= yMax; y++)
	    for (z = zMin; z <= zMax; z++)
		if (!G3d_flushTile(map, G3d_tile2tileIndex(map, x, y, z))) {
		    G3d_error("G3d_flushTileCube: error in G3d_flushTile");
		    return 0;
		}

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Writes those tiles for which
 * <em>every</em> cell has coordinate contained in the axis-parallel cube
 * defined by the vertices with cell-coordinates <em>(xMin, yMin, zMin)</em>
 * and <em>(xMax, yMax, zMax)</em>.
 * Tiles which are not stored in the cache are written as NULL-tiles.
 * Write attempts for tiles which have already been written earlier are
 * ignored.
 *
 *  \param map
 *  \param xMin
 *  \param yMin
 *  \param zMin
 *  \param xMax
 *  \param yMax
 *  \param zMax
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int
G3d_flushTilesInCube(RASTER3D_Map * map, int xMin, int yMin, int zMin, int xMax,
		     int yMax, int zMax)
{
    int xTileMin, yTileMin, zTileMin, xTileMax, yTileMax, zTileMax;
    int xOffs, yOffs, zOffs;
    int regionMaxX, regionMaxY, regionMaxZ;

    if (!map->useCache)
	G3d_fatalError
	    ("G3d_flushTilesInCube: function invalid in non-cache mode");
     /*AV*/
	/*BEGIN OF ORIGINAL CODE */
	/*
	 *  G3d_getCoordsMap (map, &regionMaxX, &regionMaxY, &regionMaxZ);
	 */
	 /*AV*/
	/* BEGIN OF MY CODE */
	G3d_getCoordsMap(map, &regionMaxY, &regionMaxX, &regionMaxZ);
    /* END OF MY CODE */

    if ((xMin < 0) && (xMax < 0))
	G3d_fatalError("G3d_flushTilesInCube: coordinate out of Range");
    if ((xMin >= regionMaxX) && (xMax >= regionMaxX))
	G3d_fatalError("G3d_flushTilesInCube: coordinate out of Range");

    xMin = MIN(MAX(0, xMin), regionMaxX - 1);

    if ((yMin < 0) && (yMax < 0))
	G3d_fatalError("G3d_flushTilesInCube: coordinate out of Range");
    if ((yMin >= regionMaxY) && (yMax >= regionMaxY))
	G3d_fatalError("G3d_flushTilesInCube: coordinate out of Range");

    yMin = MIN(MAX(0, yMin), regionMaxY - 1);

    if ((zMin < 0) && (zMax < 0))
	G3d_fatalError("G3d_flushTilesInCube: coordinate out of Range");
    if ((zMin >= regionMaxZ) && (zMax >= regionMaxZ))
	G3d_fatalError("G3d_flushTilesInCube: coordinate out of Range");

    zMin = MIN(MAX(0, zMin), regionMaxZ - 1);

    G3d_coord2tileCoord(map, xMin, yMin, zMin,
			&xTileMin, &yTileMin, &zTileMin,
			&xOffs, &yOffs, &zOffs);

    if (xOffs != 0)
	xTileMin++;
    if (yOffs != 0)
	yTileMin++;
    if (zOffs != 0)
	zTileMin++;

    G3d_coord2tileCoord(map, xMax + 1, yMax + 1, zMax + 1,
			&xTileMax, &yTileMax, &zTileMax,
			&xOffs, &yOffs, &zOffs);

    xTileMax--;
    yTileMax--;
    zTileMax--;

    if (!G3d_flushTileCube(map, xTileMin, yTileMin, zTileMin,
			   xTileMax, yTileMax, zTileMax)) {
	G3d_error("G3d_flushTilesInCube: error in G3d_flushTileCube");
	return 0;
    }

    return 1;
}

#undef MIN
#undef MAX

/*---------------------------------------------------------------------------*/

