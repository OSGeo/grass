
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Makes a copy of <em>map</em> with name <em>nameOut</em> in which the
 * cells are of type FCELL_TYPE if they are DCELL_TYPE in <em>map</em>,
 * and in DCELL_TYPE otherwise.
 * The source code can be found in <em>changetype.c</em>.
 *
 *  \param map
 *  \param nameOut
 *  \return void
 */

void Rast3d_change_type(void *map, const char *nameOut)
{
    void *map2;
    int x, y, z, saveType;
    void *data, *data2;
    RASTER3D_Region region;
    int tileSize;
    int tileX, tileY, tileZ, typeIntern, typeIntern2;
    int tileXsave, tileYsave, tileZsave, nx, ny, nz;

    saveType = Rast3d_get_file_type();
    Rast3d_set_file_type(Rast3d_file_type_map(map) == FCELL_TYPE ?
		    DCELL_TYPE : FCELL_TYPE);
    Rast3d_get_tile_dimension(&tileXsave, &tileYsave, &tileZsave);
    Rast3d_get_tile_dimensions_map(map, &tileX, &tileY, &tileZ);
    Rast3d_set_tile_dimension(tileX, tileY, tileZ);

    Rast3d_get_region_struct_map(map, &region);
    map2 =
	Rast3d_open_cell_new(nameOut, FCELL_TYPE, RASTER3D_USE_CACHE_DEFAULT, &region);

    if (map2 == NULL)
	Rast3d_fatal_error("Rast3d_change_type: error in Rast3d_open_cell_new");

    Rast3d_set_file_type(saveType);
    Rast3d_set_tile_dimension(tileXsave, tileYsave, tileZsave);

    data = Rast3d_alloc_tiles(map, 1);
    if (data == NULL)
	Rast3d_fatal_error("Rast3d_change_type: error in Rast3d_alloc_tiles");
    data2 = Rast3d_alloc_tiles(map2, 1);
    if (data2 == NULL)
	Rast3d_fatal_error("Rast3d_change_type: error in Rast3d_alloc_tiles");

    Rast3d_get_nof_tiles_map(map2, &nx, &ny, &nz);
    typeIntern = Rast3d_tile_type_map(map);
    typeIntern2 = Rast3d_tile_type_map(map2);
    tileSize = tileX * tileY * tileZ;

    for (z = 0; z < nz; z++)
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		if (!Rast3d_read_tile(map, Rast3d_tile2tile_index(map, x, y, z), data,
				  typeIntern))
		    Rast3d_fatal_error("Rast3d_change_type: error in Rast3d_read_tile");
		Rast3d_copy_values(data, 0, typeIntern, data2, 0, typeIntern2,
			       tileSize);
		if (!Rast3d_write_tile
		    (map2, Rast3d_tile2tile_index(map2, x, y, z), data2,
		     typeIntern2))
		    Rast3d_fatal_error("Rast3d_change_type: error in Rast3d_write_tile");
	    }

    Rast3d_free_tiles(data);
    Rast3d_free_tiles(data2);
    if (!Rast3d_close(map2))
	Rast3d_fatal_error("Rast3d_change_type: error in Rast3d_close");
}
