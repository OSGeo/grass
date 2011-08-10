
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

void G3d_changeType(void *map, const char *nameOut)
{
    void *map2;
    int x, y, z, saveType;
    void *data, *data2;
    G3D_Region region;
    int tileSize;
    int tileX, tileY, tileZ, typeIntern, typeIntern2;
    int tileXsave, tileYsave, tileZsave, nx, ny, nz;

    saveType = G3d_getFileType();
    G3d_setFileType(G3d_fileTypeMap(map) == FCELL_TYPE ?
		    DCELL_TYPE : FCELL_TYPE);
    G3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    G3d_getTileDimensionsMap(map, &tileX, &tileY, &tileZ);
    G3d_setTileDimension(tileX, tileY, tileZ);

    G3d_getRegionStructMap(map, &region);
    map2 =
	G3d_openCellNew(nameOut, FCELL_TYPE, G3D_USE_CACHE_DEFAULT, &region);

    if (map2 == NULL)
	G3d_fatalError("G3d_changeType: error in G3d_openCellNew");

    G3d_setFileType(saveType);
    G3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    data = G3d_allocTiles(map, 1);
    if (data == NULL)
	G3d_fatalError("G3d_changeType: error in G3d_allocTiles");
    data2 = G3d_allocTiles(map2, 1);
    if (data2 == NULL)
	G3d_fatalError("G3d_changeType: error in G3d_allocTiles");

    G3d_getNofTilesMap(map2, &nx, &ny, &nz);
    typeIntern = G3d_tileTypeMap(map);
    typeIntern2 = G3d_tileTypeMap(map2);
    tileSize = tileX * tileY * tileZ;

    for (z = 0; z < nz; z++)
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		if (!G3d_readTile(map, G3d_tile2tileIndex(map, x, y, z), data,
				  typeIntern))
		    G3d_fatalError("G3d_changeType: error in G3d_readTile");
		G3d_copyValues(data, 0, typeIntern, data2, 0, typeIntern2,
			       tileSize);
		if (!G3d_writeTile
		    (map2, G3d_tile2tileIndex(map2, x, y, z), data2,
		     typeIntern2))
		    G3d_fatalError("G3d_changeType: error in G3d_writeTile");
	    }

    G3d_freeTiles(data);
    G3d_freeTiles(data2);
    if (!G3d_closeCell(map2))
	G3d_fatalError("G3d_changeType: error in G3d_closeCell");
}
