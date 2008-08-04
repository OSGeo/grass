#include <grass/gis.h>
#include <grass/G3d.h>

/*----------------------------------------------------------------------------*/

void *G3d_openNewParam(const char *name, int typeIntern, int cache,
		       G3D_Region * region, int type, int doLzw, int doRle,
		       int precision, int tileX, int tileY, int tileZ)
{
    void *map;
    int oldCompress, oldLzw, oldRle, oldPrecision, oldTileX, oldTileY,
	oldTileZ;
    int oldType;

    G3d_initDefaults();

    G3d_getCompressionMode(&oldCompress, &oldLzw, &oldRle, &oldPrecision);
    G3d_setCompressionMode(oldCompress, doLzw, doRle, precision);

    G3d_getTileDimension(&oldTileX, &oldTileY, &oldTileZ);
    G3d_setTileDimension(tileX, tileY, tileZ);

    oldType = G3d_getFileType();
    G3d_setFileType(type);

    map = G3d_openCellNew(name, typeIntern, cache, region);

    G3d_setCompressionMode(oldCompress, oldLzw, oldRle, oldPrecision);
    G3d_setTileDimension(oldTileX, oldTileY, oldTileZ);
    G3d_setFileType(oldType);

    return map;
}
