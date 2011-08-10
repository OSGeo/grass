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

void G3d_changePrecision(void *map, int precision, const char *nameOut)
{
    void *map2;
    int x, y, z, savePrecision, saveCompression, saveLzw, saveRle;
    char *data;
    G3D_Region region;
    int typeIntern;
    int nx, ny, nz;
    int tileXsave, tileYsave, tileZsave, tileX, tileY, tileZ, saveType;

    saveType = G3d_getFileType();
    /*   G3d_setFileType (G3d_fileTypeMap (map)); */
    G3d_getCompressionMode(&saveCompression, &saveLzw, &saveRle,
			   &savePrecision);
    G3d_setCompressionMode(G3D_COMPRESSION, saveLzw, saveRle, precision);
    G3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    G3d_getTileDimensionsMap(map, &tileX, &tileY, &tileZ);
    G3d_setTileDimension(tileX, tileY, tileZ);

    typeIntern = G3d_tileTypeMap(map);
    G3d_getRegionStructMap(map, &region);

    map2 =
	G3d_openCellNew(nameOut, typeIntern, G3D_USE_CACHE_DEFAULT, &region);
    if (map2 == NULL)
	G3d_fatalError("G3d_changePrecision: error in G3d_openCellNew");

    G3d_setFileType(saveType);
    G3d_setCompressionMode(saveCompression, saveLzw, saveRle, savePrecision);
    G3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    data = G3d_allocTiles(map, 1);
    if (data == NULL)
	G3d_fatalError("G3d_changePrecision: error in G3d_allocTiles");
    G3d_getNofTilesMap(map2, &nx, &ny, &nz);

    for (z = 0; z < nz; z++)
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		if (!G3d_readTile(map, G3d_tile2tileIndex(map, x, y, z), data,
				  typeIntern))
		    G3d_fatalError
			("G3d_changePrecision: error in G3d_readTile");
		if (!G3d_writeTile
		    (map2, G3d_tile2tileIndex(map2, x, y, z), data,
		     typeIntern))
		    G3d_fatalError
			("G3d_changePrecision: error in G3d_writeTile");
	    }

    G3d_freeTiles(data);
    if (!G3d_closeCell(map2))
	G3d_fatalError("G3d_changePrecision: error in G3d_closeCell");
}
