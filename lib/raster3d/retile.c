#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>

/*---------------------------------------------------------------------------*/

static void
retileNocache(void *map, const char *nameOut, int tileX, int tileY, int tileZ)
{
    void *map2;
    int x, y, z, saveType, nx, ny, nz;
    int typeIntern;
    void *data;
    int tileXsave, tileYsave, tileZsave;
    RASTER3D_Region region;

    saveType = Rast3d_getFileType();
    Rast3d_setFileType(Rast3d_fileTypeMap(map));
    Rast3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    Rast3d_setTileDimension(tileX, tileY, tileZ);
    typeIntern = Rast3d_tileTypeMap(map);
    Rast3d_getRegionStructMap(map, &region);

    map2 = Rast3d_openCellNew(nameOut, typeIntern, RASTER3D_NO_CACHE, &region);

    if (map2 == NULL)
	Rast3d_fatalError("Rast3d_retile: error in Rast3d_openCellNew");

    Rast3d_setFileType(saveType);
    Rast3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    data = Rast3d_allocTiles(map2, 1);
    if (data == NULL)
	Rast3d_fatalError("Rast3d_retile: error in Rast3d_allocTiles");

    Rast3d_getNofTilesMap(map2, &nx, &ny, &nz);

    for (z = 0; z < nz; z++) {
        G_percent(z, nz, 1);
	for (y = 0; y < ny; y++)
	    for (x = 0; x < nx; x++) {
		Rast3d_getBlock(map, x * tileX, y * tileY, z * tileZ,
			     tileX, tileY, tileZ, data, typeIntern);
		if (!Rast3d_writeTile
		    (map2, Rast3d_tile2tileIndex(map2, x, y, z), data,
		     typeIntern))
		    Rast3d_fatalError
			("Rast3d_retileNocache: error in Rast3d_writeTile");
	    }
    }
    
    G_percent(1, 1, 1);
        
    Rast3d_freeTiles(data);
    Rast3d_closeCell(map2);
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
Rast3d_retile(void *map, const char *nameOut, int tileX, int tileY, int tileZ)
{
    void *map2;
    double value;
    int x, y, z, saveType;
    int rows, cols, depths, typeIntern;
    int xTile, yTile, zTile;
    int xOffs, yOffs, zOffs, prev;
    int tileXsave, tileYsave, tileZsave;
    RASTER3D_Region region;

    if (!Rast3d_tileUseCacheMap(map)) {
	retileNocache(map, nameOut, tileX, tileY, tileZ);
	return;
    }

    saveType = Rast3d_getFileType();
    Rast3d_setFileType(Rast3d_fileTypeMap(map));
    Rast3d_getTileDimension(&tileXsave, &tileYsave, &tileZsave);
    Rast3d_setTileDimension(tileX, tileY, tileZ);

    typeIntern = Rast3d_tileTypeMap(map);
    Rast3d_getRegionStructMap(map, &region);

    map2 =
	Rast3d_openCellNew(nameOut, typeIntern, RASTER3D_USE_CACHE_DEFAULT, &region);
    if (map2 == NULL)
	Rast3d_fatalError("Rast3d_retile: error in Rast3d_openCellNew");

    Rast3d_setFileType(saveType);
    Rast3d_setTileDimension(tileXsave, tileYsave, tileZsave);

    Rast3d_coord2tileCoord(map2, 0, 0, 0,
			&xTile, &yTile, &zTile, &xOffs, &yOffs, &zOffs);

    prev = zTile;

    x = 0;
    y = 0;

    Rast3d_getCoordsMap(map, &rows, &cols, &depths);

    for (z = 0; z < depths; z++) {
        G_percent(z, depths, 1);
	Rast3d_coord2tileCoord(map2, x, y, z, &xTile, &yTile, &zTile,
			    &xOffs, &yOffs, &zOffs);
	if (zTile > prev) {
	    if (!Rast3d_flushAllTiles(map2))
		Rast3d_fatalError("Rast3d_retile: error in Rast3d_flushAllTiles");
	    prev++;
	}

	for (y = 0; y < rows; y++)
	    for (x = 0; x < cols; x++) {

		Rast3d_getValueRegion(map, x, y, z, &value, typeIntern);
		if (!Rast3d_putValue(map2, x, y, z, &value, typeIntern))
		    Rast3d_fatalError("Rast3d_retile: error in Rast3d_putValue");
	    }
    }

    G_percent(1, 1, 1);
    if (!Rast3d_flushAllTiles(map2))
	Rast3d_fatalError("Rast3d_retile: error in Rast3d_flushAllTiles");
    if (!Rast3d_closeCell(map2))
	Rast3d_fatalError("Rast3d_retile: error in Rast3d_closeCell");
}
