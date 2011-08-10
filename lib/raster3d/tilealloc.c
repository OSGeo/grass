#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Allocates a vector of <em>nofTiles</em> tiles with the same dimensions
 * as the tiles of <em>map</em> and large enough to store cell-values of
 * <em>type</em>.
 *
 *  \param map
 *  \param nofTiles
 *  \param type
 *  \return void * : a pointer to the vector ... if successful,
 *                   NULL ... otherwise.
 */

void *G3d_allocTilesType(G3D_Map * map, int nofTiles, int type)
{
    void *tiles;

    tiles = G3d_malloc(map->tileSize * G3d_length(type) * nofTiles);
    if (tiles == NULL) {
	G3d_error("G3d_allocTilesType: error in G3d_malloc");
	return NULL;
    }

    return tiles;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to G3d_allocTilesType (map, nofTiles, G3d_fileTypeMap (map)).
 *
 *  \param map
 *  \param nofTiles
 *  \return void * 
 */

void *G3d_allocTiles(G3D_Map * map, int nofTiles)
{
    void *tiles;

    tiles = G3d_allocTilesType(map, nofTiles, map->typeIntern);
    if (tiles == NULL) {
	G3d_error("G3d_allocTiles: error in G3d_allocTilesType");
	return NULL;
    }

    return tiles;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Is equivalent to <tt>G3d_free (tiles);</tt>
 *
 *  \param tiles
 *  \return void
 */

void G3d_freeTiles(void *tiles)
{
    G3d_free(tiles);
}
