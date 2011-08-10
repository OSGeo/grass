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

void Rast3d_changePrecision(void *map, int precision, const char *nameOut)
{
    void *map2;
    int x, y, z, savePrecision, saveCompression, saveLzw, saveRle;
    char *data;
    RASTER3D_Region region;
    int typeIntern;
    int nx, ny, nz;
    int tileXsave, tileYsave, tileZsave, tileX, tileY, tileZ, saveType;

    saveType = Rast3d_getFileType();
    /*   Rast3d_setFileType (Rast3d_fileTypeMap (map)); */
    Rast3d_getCompressionMode(&saveCompression, &saveLzw, &saveRle,
			   &savePrecision);
    Rast3d_setCompressionMode(RASTER3D_COMPRESSION, saveLzw, saveRle, precision);
    Rast3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    Rast3d_getTileDimensionsMap(map, &tileX, &tileY, &tileZ);
    Rast3d_setTileDimension(tileX, tileY, tileZ);

    typeIntern = Rast3d_tileTypeMap(map);
    Rast3d_getRegionStructMap(map, &region);

    map2 =
	Rast3d_openCellNew(nameOut, typeIntern, RASTER3D_USE_CACHE_DEFAULT, &region);
    if (map2 == NULL)
	Rast3d_fatalError("Rast3d_changePrecision: error in Rast3d_openCellNew");

    Rast3d_setFileType(saveType);
    Rast3d_setCompressionMode(saveCompression, saveLzw, saveRle, savePrecision);
    Rast3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    data = Rast3d_allocTiles(map, 1);
    if (data == NULL)
	Rast3d_fatalError("Rast3d_changePrecision: error in Rast3d_allocTiles");
    Rast3d_getNofTilesMap(map2, &nx, &ny, &nz);

    for (z = 0; z < nz; z++)
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		if (!Rast3d_readTile(map, Rast3d_tile2tileIndex(map, x, y, z), data,
				  typeIntern))
		    Rast3d_fatalError
			("Rast3d_changePrecision: error in Rast3d_readTile");
		if (!Rast3d_writeTile
		    (map2, Rast3d_tile2tileIndex(map2, x, y, z), data,
		     typeIntern))
		    Rast3d_fatalError
			("Rast3d_changePrecision: error in Rast3d_writeTile");
	    }

    Rast3d_freeTiles(data);
    if (!Rast3d_closeCell(map2))
	Rast3d_fatalError("Rast3d_changePrecision: error in Rast3d_closeCell");
}
