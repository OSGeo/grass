
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

		       /* EXPORTED FUNCTIONS */

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * This function
 * returns a pointer to a tile which contains the data for the tile with index
 * <em>tileIndex</em>.  The type of the data stored in the tile depends on the type
 * specified at the initialization time of <em>map</em>.  The functionality is
 * different depending on whether <em>map</em> is old or new and depending on the
 * cache-mode of <em>map</em>.<br>
 * 
 * If <em>map</em> is old and the cache is not used the tile with <em>tileIndex</em>
 * is read from file and stored in the buffer provided by the map structure.
 * The pointer to this buffer is returned. If the buffer already contains the
 * tile with <em>tileIndex</em> reading is skipped. Data which was stored in
 * earlier calls to <tt>Rast3d_get_tile_ptr</tt> is destroyed.  
 * If the tile with <em>tileIndex</em> is not stored on the file corresponding 
 * to <em>map</em>, and <em>tileIndex</em> is a valid index the buffer is filled with NULL-values.<br>
 * 
 * If <em>map</em> is old and the cache is used the tile with <em>tileIndex</em> is
 * read from file and stored in one of the cache buffers.  The pointer to buffer
 * is returned.  If no free cache buffer is available an unlocked cache-buffer
 * is freed up and the new tile is stored in its place.  If the tile with <em>tileIndex</em>
 * is not stored on the file corresponding to <em>map</em>, and <em>tileIndex</em> is a valid 
 * index the buffer is filled with NULL-values.  If one
 * of the cache buffers already contains the tile with <em>tileIndex</em> reading
 * is skipped and the pointer to this buffer is returned.<br>
 * 
 * If <em>map</em> is new and the cache is not used the functionality is the same
 * as if <em>map</em> is old and the cache is not used.  If the tile with <em>tileIndex</em> 
 * is already stored on file, it is read into the buffer, if not,
 * the cells are set to null-values.  If the buffer corresponding to the pointer
 * is used for writing, subsequent calls to <tt>Rast3d_get_tile_ptr</tt> may destroy the
 * values already stored in the buffer.  Use <tt>Rast3d_flush_tile</tt> to write the buffer
 * to the file before reusing it for a different index.  The use of this buffer
 * as write buffer is discouraged.<br>
 * 
 * If <em>map</em> is new and the cache is used the functionality is the same as if
 * <em>map</em> is old and the cache is used with the following exception.  If <em>tileIndex</em> 
 * is a valid index and the tile with this index is not found in
 * the cache and is not stored on the file corresponding to <em>map</em>, then the
 * file cache is queried next. If the file-cache contains the tile it is loaded
 * into the cache (memory-cache). Only if the file-cache does not contain the
 * tile it is filled with NULL-values.  Tile contents of buffers are never
 * destroyed. If a cache buffer needs to be freed up, and the tile stored in the
 * buffer has not been written to the file corresponding to <em>map</em> yet, the
 * tile is copied into the file-cache.<br>
 * 
 * Care has to be taken if this function is used in non-cache mode since it is
 * implicitly invoked every time a read or write request is issued.  The only
 * I/O-functions for which it is safe to assume that they do not invoke
 * <tt>Rast3d_get_tile_ptr</tt> are <tt>Rast3d_read_tile()</tt> and
 * <tt>Rast3d_write_tile()</tt> and their corresponding type-specific versions.
 *
 *  \param map
 *  \param tileIndex
 *  \return void * a pointer to a buffer ... if successful,
 *                 NULL ... otherwise.
 */

void *Rast3d_get_tile_ptr(RASTER3D_Map * map, int tileIndex)
{
    void *ptr;

    if ((tileIndex >= map->nTiles) || (tileIndex < 0)) {
	Rast3d_error("Rast3d_get_tile_ptr: tileIndex out of range");
	return NULL;
    }

    if (map->useCache) {
	ptr = Rast3d_cache_elt_ptr(map->cache, tileIndex);
	if (ptr == NULL) {
	    Rast3d_error("Rast3d_get_tile_ptr: error in Rast3d_cache_elt_ptr");
	    return NULL;
	}
	return ptr;
    }


    if (map->currentIndex == tileIndex)
	return map->data;

    map->currentIndex = tileIndex;
    if (!Rast3d_read_tile(map, map->currentIndex, map->data, map->typeIntern)) {
	Rast3d_error("Rast3d_get_tile_ptr: error in Rast3d_read_tile");
	return NULL;
    }


    return map->data;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Same functionality as <tt>Rast3d_get_tile_ptr()</tt> but does not return the pointer.
 *
 *  \param map
 *  \param tileIndex
 *  \return 1 ... if successful,
 *          0 ... otherwise.
 */

int Rast3d_tile_load(RASTER3D_Map * map, int tileIndex)
{
    if (Rast3d_get_tile_ptr(map, tileIndex) == NULL) {
	Rast3d_error("Rast3d_tile_load: error in Rast3d_get_tile_ptr");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d__remove_tile(RASTER3D_Map * map, int tileIndex)
{
    if (!map->useCache)
	return 1;

    if (!Rast3d_cache_remove_elt(map->cache, tileIndex)) {
	Rast3d_error("Rast3d_removeTile: error in Rast3d_cache_remove_elt");
	return 0;
    }

    return 1;
}
