
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

void Rast3d_changeType(void *map, const char *nameOut)
{
    void *map2;
    int x, y, z, saveType;
    void *data, *data2;
    RASTER3D_Region region;
    int tileSize;
    int tileX, tileY, tileZ, typeIntern, typeIntern2;
    int tileXsave, tileYsave, tileZsave, nx, ny, nz;

    saveType = Rast3d_getFileType();
    Rast3d_setFileType(Rast3d_fileTypeMap(map) == FCELL_TYPE ?
		    DCELL_TYPE : FCELL_TYPE);
    Rast3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    Rast3d_getTileDimensionsMap(map, &tileX, &tileY, &tileZ);
    Rast3d_setTileDimension(tileX, tileY, tileZ);

    Rast3d_getRegionStructMap(map, &region);
    map2 =
	Rast3d_openCellNew(nameOut, FCELL_TYPE, RASTER3D_USE_CACHE_DEFAULT, &region);

    if (map2 == NULL)
	Rast3d_fatalError("Rast3d_changeType: error in Rast3d_openCellNew");

    Rast3d_setFileType(saveType);
    Rast3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    data = Rast3d_allocTiles(map, 1);
    if (data == NULL)
	Rast3d_fatalError("Rast3d_changeType: error in Rast3d_allocTiles");
    data2 = Rast3d_allocTiles(map2, 1);
    if (data2 == NULL)
	Rast3d_fatalError("Rast3d_changeType: error in Rast3d_allocTiles");

    Rast3d_getNofTilesMap(map2, &nx, &ny, &nz);
    typeIntern = Rast3d_tileTypeMap(map);
    typeIntern2 = Rast3d_tileTypeMap(map2);
    tileSize = tileX * tileY * tileZ;

    for (z = 0; z < nz; z++)
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		if (!Rast3d_readTile(map, Rast3d_tile2tileIndex(map, x, y, z), data,
				  typeIntern))
		    Rast3d_fatalError("Rast3d_changeType: error in Rast3d_readTile");
		Rast3d_copyValues(data, 0, typeIntern, data2, 0, typeIntern2,
			       tileSize);
		if (!Rast3d_writeTile
		    (map2, Rast3d_tile2tileIndex(map2, x, y, z), data2,
		     typeIntern2))
		    Rast3d_fatalError("Rast3d_changeType: error in Rast3d_writeTile");
	    }

    Rast3d_freeTiles(data);
    Rast3d_freeTiles(data2);
    if (!Rast3d_closeCell(map2))
	Rast3d_fatalError("Rast3d_changeType: error in Rast3d_closeCell");
}
