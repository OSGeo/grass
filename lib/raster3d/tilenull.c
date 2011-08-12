#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Assumes that <em>tile</em> is a tile with the same dimensions as the
 * tiles of <em>map</em>. Fills <em>tile</em> with NULL-values of
 * <em>type</em>.
 *
 *  \param map
 *  \param tile
 *  \param type
 *  \return void
 */

void Rast3d_set_null_tile_type(RASTER3D_Map * map, void *tile, int type)
{
    Rast3d_set_null_value(tile, map->tileSize, type);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Is equivalent to Rast3d_set_null_tile_type (map, tile, Rast3d_file_type_map (map)).
 *
 *  \param map
 *  \param tile
 *  \return void
 */

void Rast3d_set_null_tile(RASTER3D_Map * map, void *tile)
{
    Rast3d_set_null_tile_type(map, tile, map->typeIntern);
}
