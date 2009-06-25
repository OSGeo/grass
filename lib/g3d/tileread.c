#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include <grass/raster.h>
#include "G3d_intern.h"

static int
G3d_xdrTile2tile(G3D_Map * map, void *tile, int rows, int cols, int depths,
		 int xRedundant, int yRedundant, int zRedundant, int nofNum,
		 int type)
{
    int y, z, xLength, yLength, length;

    if (!G3d_initCopyFromXdr(map, type)) {
	G3d_error("G3d_xdrTile2tile: error in G3d_initCopyFromXdr");
	return 0;
    }

    if (nofNum == map->tileSize) {
	if (!G3d_copyFromXdr(map->tileSize, tile)) {
	    G3d_error("G3d_xdrTile2tile: error in G3d_copyFromXdr");
	    return 0;
	}
	return 1;
    }

    length = G3d_length(type);
    xLength = xRedundant * length;
    yLength = map->tileX * yRedundant * length;

    if (xRedundant) {
	for (z = 0; z < depths; z++) {
	    for (y = 0; y < rows; y++) {
		if (!G3d_copyFromXdr(cols, tile)) {
		    G3d_error("G3d_xdrTile2tile: error in G3d_copyFromXdr");
		    return 0;
		}
		tile = G_incr_void_ptr(tile, cols * length);
		G3d_setNullValue(tile, xRedundant, type);
		tile = G_incr_void_ptr(tile, xLength);
	    }
	    if (yRedundant) {
		G3d_setNullValue(tile, map->tileX * yRedundant, type);
		tile = G_incr_void_ptr(tile, yLength);
	    }
	}
	if (!zRedundant)
	    return 1;

	G3d_setNullValue(tile, map->tileXY * zRedundant, type);
	return 1;
    }

    if (yRedundant) {
	for (z = 0; z < depths; z++) {
	    if (!G3d_copyFromXdr(map->tileX * rows, tile)) {
		G3d_error("G3d_xdrTile2tile: error in G3d_copyFromXdr");
		return 0;
	    }
	    tile = G_incr_void_ptr(tile, map->tileX * rows * length);
	    G3d_setNullValue(tile, map->tileX * yRedundant, type);
	    tile = G_incr_void_ptr(tile, yLength);
	}
	if (!zRedundant)
	    return 1;

	G3d_setNullValue(tile, map->tileXY * zRedundant, type);
	return 1;
    }

    if (!G3d_copyFromXdr(map->tileXY * depths, tile)) {
	G3d_error("G3d_xdrTile2tile: error in G3d_copyFromXdr");
	return 0;
    }

    if (!zRedundant)
	return 1;

    tile = G_incr_void_ptr(tile, map->tileXY * depths * length);
    G3d_setNullValue(tile, map->tileXY * zRedundant, type);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_readTileUncompressed(G3D_Map * map, int tileIndex, int nofNum)
{
    int nofBytes;

    nofBytes = nofNum * map->numLengthExtern;
    nofBytes = G3D_MIN(nofBytes, map->fileEndPtr - map->index[tileIndex]);

    if (read(map->data_fd, xdr, nofBytes) != nofBytes) {
	G3d_error("G3d_readTileUncompressed: can't read file");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_readTileCompressed(G3D_Map * map, int tileIndex, int nofNum)
{
    if (!G_fpcompress_readXdrNums(map->data_fd, xdr, nofNum,
				  map->tileLength[tileIndex],
				  map->precision, tmpCompress,
				  map->type == FCELL_TYPE)) {
	G3d_error
	    ("G3d_readTileCompressed: error in G_fpcompress_readXdrNums");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
		       /* EXPORTED FUNCTIONS */

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 * 
 * Reads tile with index <em>tileIndex</em> into the <em>tile</em> buffer. The cells
 * are stored with type <em>type</em> which must be one of FCELL_TYPE and
 * DCELL_TYPE. If the tile with <em>tileIndex</em> is not stored on the file
 * corresponding to <em>map</em>, and <em>tileIndex</em> is a valid index <em>tile</em>
 * is filled with NULL-values. 
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \param type
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int G3d_readTile(G3D_Map * map, int tileIndex, void *tile, int type)
{
    int nofNum, rows, cols, depths, xRedundant, yRedundant, zRedundant;

    if ((tileIndex >= map->nTiles) || (tileIndex < 0))
	G3d_fatalError("G3d_readTile: tile index out of range");

    if (map->index[tileIndex] == -1) {
	G3d_setNullTileType(map, tile, type);
	return 1;
    }

    nofNum = G3d_computeClippedTileDimensions(map, tileIndex,
					      &rows, &cols, &depths,
					      &xRedundant, &yRedundant,
					      &zRedundant);

    if (lseek(map->data_fd, map->index[tileIndex], SEEK_SET) == -1) {
	G3d_error("G3d_readTile: can't position file");
	return 0;
    }

    if (map->compression == G3D_NO_COMPRESSION) {
	if (!G3d_readTileUncompressed(map, tileIndex, nofNum)) {
	    G3d_error("G3d_readTile: error in G3d_readTileUncompressed");
	    return 0;
	}
    }
    else if (!G3d_readTileCompressed(map, tileIndex, nofNum)) {
	G3d_error("G3d_readTile: error in G3d_readTileCompressed");
	return 0;
    }

    if (!G3d_xdrTile2tile(map, tile, rows, cols, depths,
			  xRedundant, yRedundant, zRedundant, nofNum, type)) {
	G3d_error("G3d_readTile: error in G3d_xdrTile2tile");
	return 0;
    }

    if (G3d_maskIsOff(map))
	return 1;

    G3d_maskTile(map, tileIndex, tile, type);
    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to G3d_readTile (map, tileIndex, tile, FCELL_TYPE).
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \return int
 */

int G3d_readTileFloat(G3D_Map * map, int tileIndex, void *tile)
{
    if (!G3d_readTile(map, tileIndex, tile, FCELL_TYPE)) {
	G3d_error("G3d_readTileFloat: error in G3d_readTile");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to G3d_readTile (map, tileIndex, tile, DCELL_TYPE).
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \return int
 */

int G3d_readTileDouble(G3D_Map * map, int tileIndex, void *tile)
{
    if (!G3d_readTile(map, tileIndex, tile, DCELL_TYPE)) {
	G3d_error("G3d_readTileDouble: error in G3d_readTile");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

		      /* CACHE-MODE-ONLY FUNCTIONS */

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Locks tile with <em>tileIndex</em> in cache. If after locking fewer than the minimum number of
 *  unlocked tiles are unlocked, the lock request is ignored.
 *
 *  \param map
 *  \param tileIndex
 *  \return 1 ... if successful,
 *          -1 ... if request is ignored,
 *          0 ... otherwise.
 */

int G3d_lockTile(G3D_Map * map, int tileIndex)
{
    if (!map->useCache)
	G3d_fatalError("G3d_lockTile: function invalid in non-cache mode");

    if (!G3d_cache_lock(map->cache, tileIndex)) {
	G3d_error("G3d_lockTile: error in G3d_cache_lock");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Unlocks tile with <em>tileIndex</em>.
 *
 *  \param map
 *  \param tileIndex
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int G3d_unlockTile(G3D_Map * map, int tileIndex)
{
    if (!map->useCache)
	G3d_fatalError("G3d_unlockTile: function invalid in non-cache mode");

    if (!G3d_cache_unlock(map->cache, tileIndex)) {
	G3d_error("G3d_unlockTile: error in G3d_cache_unlock");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Unlocks every tile in cache of <em>map</em>.
 *
 *  \param map
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int G3d_unlockAll(G3D_Map * map)
{
    if (!map->useCache)
	G3d_fatalError("G3d_unlockAll: function invalid in non-cache mode");

    if (!G3d_cache_unlock_all(map->cache)) {
	G3d_error("G3d_unlockAll: error in G3d_cache_unlock_all");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Turns autolock mode on.
 *
 *  \param map
 *  \return void
 */

void G3d_autolockOn(G3D_Map * map)
{
    if (!map->useCache)
	G3d_fatalError("G3d_autoLockOn: function invalid in non-cache mode");

    G3d_cache_autolock_on(map->cache);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Turns autolock mode Off.
 *
 *  \param map
 *  \return void
 */

void G3d_autolockOff(G3D_Map * map)
{
    if (!map->useCache)
	G3d_fatalError("G3d_autoLockOff: function invalid in non-cache mode");

    G3d_cache_autolock_off(map->cache);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the minimum
 * number of unlocked tiles to <em>minUnlocked</em>.  This function should be used
 * in combination with <tt>G3d_unlockAll ()</tt> in order to avoid situations where the
 * new minimum is larger than the actual number of unlocked tiles.
 * <em>minUnlocked</em> must be one of G3D_USE_CACHE_X, G3D_USE_CACHE_Y,
 * G3D_USE_CACHE_Z, G3D_USE_CACHE_XY, G3D_USE_CACHE_XZ,
 * G3D_USE_CACHE_YZ, G3D_USE_CACHE_XYZ, the result of G3d_cacheSizeEncode()
 * (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer
 * which explicitly specifies the number of tiles.
 *
 *  \param map
 *  \param minUnlocked
 *  \return void
 */

void G3d_minUnlocked(G3D_Map * map, int minUnlocked)
{
    if (!map->useCache)
	G3d_fatalError("G3d_autoLockOff: function invalid in non-cache mode");

    G3d_cache_set_minUnlock(map->cache,
			    G3d__computeCacheSize(map, minUnlocked));
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Starts a new cycle.
 *
 *  \param map
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int G3d_beginCycle(G3D_Map * map)
{
    if (!G3d_unlockAll(map)) {
	G3d_fatalError("G3d_beginCycle: error in G3d_unlockAll");
	return 0;
    }

    G3d_autolockOn(map);
    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Ends a cycle.
 *
 *  \param map
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int G3d_endCycle(G3D_Map * map)
{
    G3d_autolockOff(map);
    return 1;
}
