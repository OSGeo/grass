#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Makes a copy of <em>map</em> with name <em>nameOut</em> which is
 *  written with <em>precision</em>.
 * The source code can be found in <em>changeprecision.c</em>.
 *
 *  \param map
 *  \param precision
 *  \param nameOut
 *  \return void
 */

void Rast3d_change_precision(void *map, int precision, const char *nameOut)
{
    void *map2;
    int x, y, z, savePrecision, saveCompression;
    char *data;
    RASTER3D_Region region;
    int typeIntern;
    int nx, ny, nz;
    int tileXsave, tileYsave, tileZsave, tileX, tileY, tileZ, saveType;

    saveType = Rast3d_get_file_type();
    /*   Rast3d_set_file_type (Rast3d_file_type_map (map)); */
    Rast3d_get_compression_mode(&saveCompression, &savePrecision);
    Rast3d_set_compression_mode(RASTER3D_COMPRESSION, precision);
    Rast3d_get_tile_dimension(&tileXsave, &tileYsave, &tileZsave);
    Rast3d_get_tile_dimensions_map(map, &tileX, &tileY, &tileZ);
    Rast3d_set_tile_dimension(tileX, tileY, tileZ);

    typeIntern = Rast3d_tile_type_map(map);
    Rast3d_get_region_struct_map(map, &region);

    map2 =
	Rast3d_open_cell_new(nameOut, typeIntern, RASTER3D_USE_CACHE_DEFAULT, &region);
    if (map2 == NULL)
	Rast3d_fatal_error("Rast3d_change_precision: error in Rast3d_open_cell_new");

    Rast3d_set_file_type(saveType);
    Rast3d_set_compression_mode(saveCompression, savePrecision);
    Rast3d_set_tile_dimension(tileXsave, tileYsave, tileZsave);

    data = Rast3d_alloc_tiles(map, 1);
    if (data == NULL)
	Rast3d_fatal_error("Rast3d_change_precision: error in Rast3d_alloc_tiles");
    Rast3d_get_nof_tiles_map(map2, &nx, &ny, &nz);

    for (z = 0; z < nz; z++)
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		if (!Rast3d_read_tile(map, Rast3d_tile2tile_index(map, x, y, z), data,
				  typeIntern))
		    Rast3d_fatal_error
			("Rast3d_change_precision: error in Rast3d_read_tile");
		if (!Rast3d_write_tile
		    (map2, Rast3d_tile2tile_index(map2, x, y, z), data,
		     typeIntern))
		    Rast3d_fatal_error
			("Rast3d_change_precision: error in Rast3d_write_tile");
	    }

    Rast3d_free_tiles(data);
    if (!Rast3d_close(map2))
	Rast3d_fatal_error("Rast3d_change_precision: error in Rast3d_close");
}
