#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/G3d.h>

/*---------------------------------------------------------------------------*/

static void
retileNocache(void *map, const char *nameOut, int tileX, int tileY, int tileZ)
{
    void *map2;
    int x, y, z, saveType, nx, ny, nz;
    int typeIntern;
    void *data;
    int tileXsave, tileYsave, tileZsave;
    G3D_Region region;

    saveType = G3d_getFileType();
    G3d_setFileType(G3d_fileTypeMap(map));
    G3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    G3d_setTileDimension(tileX, tileY, tileZ);
    typeIntern = G3d_tileTypeMap(map);
    G3d_getRegionStructMap(map, &region);

    map2 = G3d_openCellNew(nameOut, typeIntern, G3D_NO_CACHE, &region);

    if (map2 == NULL)
	G3d_fatalError("G3d_retile: error in G3d_openCellNew");

    G3d_setFileType(saveType);
    G3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    data = G3d_allocTiles(map2, 1);
    if (data == NULL)
	G3d_fatalError("G3d_retile: error in G3d_allocTiles");

    G3d_getNofTilesMap(map2, &nx, &ny, &nz);

    for (z = 0; z < nz; z++) {
        G_percent(z, nz, 1);
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		G3d_getBlock(map, x * tileX, y * tileY, z * tileZ,
			     tileX, tileY, tileZ, data, typeIntern);
		if (!G3d_writeTile
		    (map2, G3d_tile2tileIndex(map2, x, y, z), data,
		     typeIntern))
		    G3d_fatalError
			("G3d_retileNocache: error in G3d_writeTile");
	    }
    }
    
    G_percent(1, 1, 1);
        
    G3d_freeTiles(data);
    G3d_closeCell(map2);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Makes a copy of <em>map</em> with name <em>nameOut</em> which has
 * tile dimensions <em>tileX</em>, <em>tileY</em>, <em>tileZ</em>.
 * The source code can be found in <em>retile.c</em>.
 *
 *  \param map
 *  \param nameOut
 *  \param tileX
 *  \param tileY
 *  \param tileZ
 *  \return void
 */

void
G3d_retile(void *map, const char *nameOut, int tileX, int tileY, int tileZ)
{
    void *map2;
    double value;
    int x, y, z, saveType;
    int rows, cols, depths, typeIntern;
    int xTile, yTile, zTile;
    int xOffs, yOffs, zOffs, prev;
    int tileXsave, tileYsave, tileZsave;
    G3D_Region region;

    if (!G3d_tileUseCacheMap(map)) {
	retileNocache(map, nameOut, tileX, tileY, tileZ);
	return;
    }

    saveType = G3d_getFileType();
    G3d_setFileType(G3d_fileTypeMap(map));
    G3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    G3d_setTileDimension(tileX, tileY, tileZ);

    typeIntern = G3d_tileTypeMap(map);
    G3d_getRegionStructMap(map, &region);

    map2 =
	G3d_openCellNew(nameOut, typeIntern, G3D_USE_CACHE_DEFAULT, &region);
    if (map2 == NULL)
	G3d_fatalError("G3d_retile: error in G3d_openCellNew");

    G3d_setFileType(saveType);
    G3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    G3d_coord2tileCoord(map2, 0, 0, 0,
			&xTile, &yTile, &zTile, &xOffs, &yOffs, &zOffs);

    prev = zTile;

    x = 0;
    y = 0;

    G3d_getCoordsMap(map, &rows, &cols, &depths);

    for (z = 0; z < depths; z++) {
        G_percent(z, depths, 1);
	G3d_coord2tileCoord(map2, x, y, z, &xTile, &yTile, &zTile,
			    &xOffs, &yOffs, &zOffs);
	if (zTile > prev) {
	    if (!G3d_flushAllTiles(map2))
		G3d_fatalError("G3d_retile: error in G3d_flushAllTiles");
	    prev++;
	}

	for (y = 0; y < rows; y++)
	    for (x = 0; x < cols; x++) {

		G3d_getValueRegion(map, x, y, z, &value, typeIntern);
		if (!G3d_putValue(map2, x, y, z, &value, typeIntern))
		    G3d_fatalError("G3d_retile: error in G3d_putValue");
	    }
    }

    G_percent(1, 1, 1);
    if (!G3d_flushAllTiles(map2))
	G3d_fatalError("G3d_retile: error in G3d_flushAllTiles");
    if (!G3d_closeCell(map2))
	G3d_fatalError("G3d_retile: error in G3d_closeCell");
}
