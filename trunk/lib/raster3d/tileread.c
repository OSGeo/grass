#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/raster.h>
#include "raster3d_intern.h"

static int
Rast3d_xdrTile2tile(RASTER3D_Map * map, void *tile, int rows, int cols, int depths,
		 int xRedundant, int yRedundant, int zRedundant, int nofNum,
		 int type)
{
    int y, z, xLength, yLength, length;

    if (!Rast3d_init_copy_from_xdr(map, type)) {
	Rast3d_error("Rast3d_xdrTile2tile: error in Rast3d_init_copy_from_xdr");
	return 0;
    }

    if (nofNum == map->tileSize) {
	if (!Rast3d_copy_from_xdr(map->tileSize, tile)) {
	    Rast3d_error("Rast3d_xdrTile2tile: error in Rast3d_copy_from_xdr");
	    return 0;
	}
	return 1;
    }

    length = Rast3d_length(type);
    xLength = xRedundant * length;
    yLength = map->tileX * yRedundant * length;

    if (xRedundant) {
	for (z = 0; z < depths; z++) {
	    for (y = 0; y < rows; y++) {
		if (!Rast3d_copy_from_xdr(cols, tile)) {
		    Rast3d_error("Rast3d_xdrTile2tile: error in Rast3d_copy_from_xdr");
		    return 0;
		}
		tile = G_incr_void_ptr(tile, cols * length);
		Rast3d_set_null_value(tile, xRedundant, type);
		tile = G_incr_void_ptr(tile, xLength);
	    }
	    if (yRedundant) {
		Rast3d_set_null_value(tile, map->tileX * yRedundant, type);
		tile = G_incr_void_ptr(tile, yLength);
	    }
	}
	if (!zRedundant)
	    return 1;

	Rast3d_set_null_value(tile, map->tileXY * zRedundant, type);
	return 1;
    }

    if (yRedundant) {
	for (z = 0; z < depths; z++) {
	    if (!Rast3d_copy_from_xdr(map->tileX * rows, tile)) {
		Rast3d_error("Rast3d_xdrTile2tile: error in Rast3d_copy_from_xdr");
		return 0;
	    }
	    tile = G_incr_void_ptr(tile, map->tileX * rows * length);
	    Rast3d_set_null_value(tile, map->tileX * yRedundant, type);
	    tile = G_incr_void_ptr(tile, yLength);
	}
	if (!zRedundant)
	    return 1;

	Rast3d_set_null_value(tile, map->tileXY * zRedundant, type);
	return 1;
    }

    if (!Rast3d_copy_from_xdr(map->tileXY * depths, tile)) {
	Rast3d_error("Rast3d_xdrTile2tile: error in Rast3d_copy_from_xdr");
	return 0;
    }

    if (!zRedundant)
	return 1;

    tile = G_incr_void_ptr(tile, map->tileXY * depths * length);
    Rast3d_set_null_value(tile, map->tileXY * zRedundant, type);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int Rast3d_readTileUncompressed(RASTER3D_Map * map, int tileIndex, int nofNum)
{
    size_t nofBytes;

    nofBytes = nofNum * map->numLengthExtern;
    nofBytes = RASTER3D_MIN(nofBytes, map->fileEndPtr - map->index[tileIndex]);

    if (read(map->data_fd, xdr, nofBytes) != nofBytes) {
	Rast3d_error("Rast3d_readTileUncompressed: can't read file");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int Rast3d_readTileCompressed(RASTER3D_Map * map, int tileIndex, int nofNum)
{
    if (!Rast3d_fpcompress_read_xdr_nums(map->data_fd, xdr, nofNum,
				  map->tileLength[tileIndex],
				  map->precision, tmpCompress,
				  map->type == FCELL_TYPE)) {
	Rast3d_error
	    ("Rast3d_readTileCompressed: error in Rast3d_fpcompress_read_xdr_nums");
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
 *          0 ... otherwise
 */

int Rast3d_read_tile(RASTER3D_Map * map, int tileIndex, void *tile, int type)
{
    int nofNum, rows, cols, depths, xRedundant, yRedundant, zRedundant;

    if ((tileIndex >= map->nTiles) || (tileIndex < 0))
	Rast3d_fatal_error("Rast3d_read_tile: tile index out of range");

    if (map->index[tileIndex] == -1) {
	Rast3d_set_null_tile_type(map, tile, type);
	return 1;
    }

    nofNum = Rast3d_compute_clipped_tile_dimensions(map, tileIndex,
					      &rows, &cols, &depths,
					      &xRedundant, &yRedundant,
					      &zRedundant);

    if (lseek(map->data_fd, map->index[tileIndex], SEEK_SET) == -1) {
	Rast3d_error("Rast3d_read_tile: can't position file");
	return 0;
    }

    if (map->compression == RASTER3D_NO_COMPRESSION) {
	if (!Rast3d_readTileUncompressed(map, tileIndex, nofNum)) {
	    Rast3d_error("Rast3d_read_tile: error in Rast3d_readTileUncompressed");
	    return 0;
	}
    }
    else if (!Rast3d_readTileCompressed(map, tileIndex, nofNum)) {
	Rast3d_error("Rast3d_read_tile: error in Rast3d_readTileCompressed");
	return 0;
    }

    if (!Rast3d_xdrTile2tile(map, tile, rows, cols, depths,
			  xRedundant, yRedundant, zRedundant, nofNum, type)) {
	Rast3d_error("Rast3d_read_tile: error in Rast3d_xdrTile2tile");
	return 0;
    }

    if (Rast3d_mask_is_off(map))
	return 1;

    Rast3d_mask_tile(map, tileIndex, tile, type);
    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to Rast3d_read_tile (map, tileIndex, tile, FCELL_TYPE).
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \return int
 */

int Rast3d_read_tile_float(RASTER3D_Map * map, int tileIndex, void *tile)
{
    if (!Rast3d_read_tile(map, tileIndex, tile, FCELL_TYPE)) {
	Rast3d_error("Rast3d_read_tile_float: error in Rast3d_read_tile");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to Rast3d_read_tile (map, tileIndex, tile, DCELL_TYPE).
 *
 *  \param map
 *  \param tileIndex
 *  \param tile
 *  \return int
 */

int Rast3d_read_tile_double(RASTER3D_Map * map, int tileIndex, void *tile)
{
    if (!Rast3d_read_tile(map, tileIndex, tile, DCELL_TYPE)) {
	Rast3d_error("Rast3d_read_tile_double: error in Rast3d_read_tile");
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

int Rast3d_lock_tile(RASTER3D_Map * map, int tileIndex)
{
    if (!map->useCache)
	Rast3d_fatal_error("Rast3d_lock_tile: function invalid in non-cache mode");

    if (!Rast3d_cache_lock(map->cache, tileIndex)) {
	Rast3d_error("Rast3d_lock_tile: error in Rast3d_cache_lock");
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

int Rast3d_unlock_tile(RASTER3D_Map * map, int tileIndex)
{
    if (!map->useCache)
	Rast3d_fatal_error("Rast3d_unlock_tile: function invalid in non-cache mode");

    if (!Rast3d_cache_unlock(map->cache, tileIndex)) {
	Rast3d_error("Rast3d_unlock_tile: error in Rast3d_cache_unlock");
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

int Rast3d_unlock_all(RASTER3D_Map * map)
{
    if (!map->useCache)
	Rast3d_fatal_error("Rast3d_unlock_all: function invalid in non-cache mode");

    if (!Rast3d_cache_unlock_all(map->cache)) {
	Rast3d_error("Rast3d_unlock_all: error in Rast3d_cache_unlock_all");
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

void Rast3d_autolock_on(RASTER3D_Map * map)
{
    if (!map->useCache)
	Rast3d_fatal_error("Rast3d_autoLockOn: function invalid in non-cache mode");

    Rast3d_cache_autolock_on(map->cache);
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

void Rast3d_autolock_off(RASTER3D_Map * map)
{
    if (!map->useCache)
	Rast3d_fatal_error("Rast3d_autoLockOff: function invalid in non-cache mode");

    Rast3d_cache_autolock_off(map->cache);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the minimum
 * number of unlocked tiles to <em>minUnlocked</em>.  This function should be used
 * in combination with <tt>Rast3d_unlock_all ()</tt> in order to avoid situations where the
 * new minimum is larger than the actual number of unlocked tiles.
 * <em>minUnlocked</em> must be one of RASTER3D_USE_CACHE_X, RASTER3D_USE_CACHE_Y,
 * RASTER3D_USE_CACHE_Z, RASTER3D_USE_CACHE_XY, RASTER3D_USE_CACHE_XZ,
 * RASTER3D_USE_CACHE_YZ, RASTER3D_USE_CACHE_XYZ, the result of Rast3d_cache_size_encode()
 * (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer
 * which explicitly specifies the number of tiles.
 *
 *  \param map
 *  \param minUnlocked
 *  \return void
 */

void Rast3d_min_unlocked(RASTER3D_Map * map, int minUnlocked)
{
    if (!map->useCache)
	Rast3d_fatal_error("Rast3d_autoLockOff: function invalid in non-cache mode");

    Rast3d_cache_set_min_unlock(map->cache,
			    Rast3d__compute_cache_size(map, minUnlocked));
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

int Rast3d_begin_cycle(RASTER3D_Map * map)
{
    if (!Rast3d_unlock_all(map)) {
	Rast3d_fatal_error("Rast3d_begin_cycle: error in Rast3d_unlock_all");
	return 0;
    }

    Rast3d_autolock_on(map);
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

int Rast3d_end_cycle(RASTER3D_Map * map)
{
    Rast3d_autolock_off(map);
    return 1;
}
