#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <grass/G3d.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

void *tmpCompress;
int tmpCompressLength;
void *xdr;
int xdrLength;

/*---------------------------------------------------------------------------*/

#define G3D_HEADER_TILEX "TileDimensionX"
#define G3D_HEADER_TILEY "TileDimensionY"
#define G3D_HEADER_TILEZ "TileDimensionZ"
#define G3D_HEADER_TYPE "CellType"
#define G3D_HEADER_COMPRESSION "useCompression"
#define G3D_HEADER_USERLE "useRle"
#define G3D_HEADER_USELZW "useLzw"
#define G3D_HEADER_PRECISION "Precision"
#define G3D_HEADER_DATA_OFFSET "nofHeaderBytes"
#define G3D_HEADER_USEXDR "useXdr"
#define G3D_HEADER_HASINDEX "hasIndex"
#define G3D_HEADER_UNIT "Units"

/*---------------------------------------------------------------------------*/

static int
G3d_readWriteHeader(struct Key_Value *headerKeys, int doRead, int *proj,
		    int *zone, double *north, double *south, double *east,
		    double *west, double *top, double *bottom, int *rows,
		    int *cols, int *depths, double *ew_res, double *ns_res,
		    double *tb_res, int *tileX, int *tileY, int *tileZ,
		    int *type, int *compression, int *useRle, int *useLzw,
		    int *precision, int *dataOffset, int *useXdr,
		    int *hasIndex, char **unit)
{
    int returnVal;
    int (*headerInt) (), (*headerDouble) (), (*headerValue) ();
    int (*headerString) ();

    if (doRead) {
	headerDouble = G3d_keyGetDouble;
	headerInt = G3d_keyGetInt;
	headerString = G3d_keyGetString;
	headerValue = G3d_keyGetValue;
    }
    else {
	headerDouble = G3d_keySetDouble;
	headerInt = G3d_keySetInt;
	headerString = G3d_keySetString;
	headerValue = G3d_keySetValue;
    }

    returnVal = 1;
    returnVal &= headerInt(headerKeys, G3D_REGION_PROJ, proj);
    returnVal &= headerInt(headerKeys, G3D_REGION_ZONE, zone);

    returnVal &= headerDouble(headerKeys, G3D_REGION_NORTH, north);
    returnVal &= headerDouble(headerKeys, G3D_REGION_SOUTH, south);
    returnVal &= headerDouble(headerKeys, G3D_REGION_EAST, east);
    returnVal &= headerDouble(headerKeys, G3D_REGION_WEST, west);
    returnVal &= headerDouble(headerKeys, G3D_REGION_TOP, top);
    returnVal &= headerDouble(headerKeys, G3D_REGION_BOTTOM, bottom);

    returnVal &= headerInt(headerKeys, G3D_REGION_ROWS, rows);
    returnVal &= headerInt(headerKeys, G3D_REGION_COLS, cols);
    returnVal &= headerInt(headerKeys, G3D_REGION_DEPTHS, depths);

    returnVal &= headerDouble(headerKeys, G3D_REGION_NSRES, ns_res);
    returnVal &= headerDouble(headerKeys, G3D_REGION_EWRES, ew_res);
    returnVal &= headerDouble(headerKeys, G3D_REGION_TBRES, tb_res);

    returnVal &= headerInt(headerKeys, G3D_HEADER_TILEX, tileX);
    returnVal &= headerInt(headerKeys, G3D_HEADER_TILEY, tileY);
    returnVal &= headerInt(headerKeys, G3D_HEADER_TILEZ, tileZ);

    returnVal &= headerValue(headerKeys, G3D_HEADER_TYPE,
			     "double", "float", DCELL_TYPE, FCELL_TYPE, type);
    returnVal &= headerValue(headerKeys, G3D_HEADER_COMPRESSION,
			     "0", "1", 0, 1, compression);
    returnVal &= headerValue(headerKeys, G3D_HEADER_USERLE,
			     "0", "1", 0, 1, useRle);
    returnVal &= headerValue(headerKeys, G3D_HEADER_USELZW,
			     "0", "1", 0, 1, useLzw);

    returnVal &= headerInt(headerKeys, G3D_HEADER_PRECISION, precision);
    returnVal &= headerInt(headerKeys, G3D_HEADER_DATA_OFFSET, dataOffset);

    returnVal &= headerValue(headerKeys, G3D_HEADER_USEXDR,
			     "0", "1", 0, 1, useXdr);
    returnVal &= headerValue(headerKeys, G3D_HEADER_HASINDEX,
			     "0", "1", 0, 1, hasIndex);
    returnVal &= headerString(headerKeys, G3D_HEADER_UNIT, unit);

    if (returnVal)
	return 1;

    G3d_error("G3d_readWriteHeader: error writing header");
    return 0;
}

/*---------------------------------------------------------------------------*/

int
G3d_readHeader(G3D_Map * map, int *proj, int *zone, double *north,
	       double *south, double *east, double *west, double *top,
	       double *bottom, int *rows, int *cols, int *depths,
	       double *ew_res, double *ns_res, double *tb_res, int *tileX,
	       int *tileY, int *tileZ, int *type, int *compression,
	       int *useRle, int *useLzw, int *precision, int *dataOffset,
	       int *useXdr, int *hasIndex, char **unit)
{
    struct Key_Value *headerKeys;
    char path[GPATH_MAX];

    G3d_filename(path, G3D_HEADER_ELEMENT, map->fileName, map->mapset);
    if (access(path, R_OK) != 0) {
	G3d_error("G3d_readHeader: unable to find [%s]", path);
	return 0;
    }

    headerKeys = G_read_key_value_file(path);

    if (!G3d_readWriteHeader(headerKeys, 1,
			     proj, zone,
			     north, south, east, west, top, bottom,
			     rows, cols, depths,
			     ew_res, ns_res, tb_res,
			     tileX, tileY, tileZ,
			     type, compression, useRle, useLzw, precision,
			     dataOffset, useXdr, hasIndex, unit)) {
	G3d_error("G3d_readHeader: error extracting header key(s) of file %s",
		  path);
	return 0;
    }

    G_free_key_value(headerKeys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_writeHeader(G3D_Map * map, int proj, int zone, double north, double south,
		double east, double west, double top, double bottom, int rows,
		int cols, int depths, double ew_res, double ns_res,
		double tb_res, int tileX, int tileY, int tileZ, int type,
		int compression, int useRle, int useLzw, int precision,
		int dataOffset, int useXdr, int hasIndex, char *unit)
{
    struct Key_Value *headerKeys;
    char path[GPATH_MAX];

    headerKeys = G_create_key_value();

    if (!G3d_readWriteHeader(headerKeys, 0,
			     &proj, &zone,
			     &north, &south, &east, &west, &top, &bottom,
			     &rows, &cols, &depths,
			     &ew_res, &ns_res, &tb_res,
			     &tileX, &tileY, &tileZ,
			     &type, &compression, &useRle, &useLzw,
			     &precision, &dataOffset, &useXdr, &hasIndex,
			     &unit)) {
	G3d_error("G3d_writeHeader: error adding header key(s) for file %s",
		  path);
	return 0;
    }

    G3d_filename(path, G3D_HEADER_ELEMENT, map->fileName, map->mapset);
    G3d_makeMapsetMapDirectory(map->fileName);
    G_write_key_value_file(path, headerKeys);

    G_free_key_value(headerKeys);

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns a number
 * which encodes multiplicity <em>n</em> of <em>cacheCode</em>. This value can be used
 * to specify the size of the cache.
 * If <em>cacheCode</em> is the size (in tiles) of the cache the function returns
 * <em>cacheCode * n</em>.
 * If <em>cacheCode</em> is G3D_USE_CACHE_DEFAULT the function returns
 * G3D_USE_CACHE_DEFAULT.
 * If <em>cacheCode</em> is G3D_USE_CACHE_??? the function returns a value
 * encoding G3D_USE_CACHE_??? and <em>n</em>. Here G3D_USE_CACHE_??? is one
 * of G3D_USE_CACHE_X, G3D_USE_CACHE_Y, G3D_USE_CACHE_Z,
 * G3D_USE_CACHE_XY, G3D_USE_CACHE_XZ, G3D_USE_CACHE_YZ, or
 * G3D_USE_CACHE_XYZ, where e.g.  G3D_USE_CACHE_X specifies that the cache
 * should store as many tiles as there exist in one row along the x-axis of the
 * tile cube, and G3D_USE_CACHE_XY specifies that the cache should store as
 * many tiles as there exist in one slice of the tile cube with constant Z
 * coordinate.
 *
 *  \param cacheCode
 *  \param n
 *  \return int
 */

int G3d_cacheSizeEncode(int cacheCode, int n)
{
    if (cacheCode >= G3D_NO_CACHE)
	return cacheCode * n;
    if (cacheCode == G3D_USE_CACHE_DEFAULT)
	return cacheCode;

    if (cacheCode < G3D_USE_CACHE_XYZ)
	G3d_fatalError("G3d_cacheSizeEncode: invalid cache code");

    return n * (-10) + cacheCode;
}

/*---------------------------------------------------------------------------*/

int G3d__computeCacheSize(G3D_Map * map, int cacheCode)
{
    int n, size;

    if (cacheCode >= G3D_NO_CACHE)
	return cacheCode;
    if (cacheCode == G3D_USE_CACHE_DEFAULT)
	return G3D_MIN(g3d_cache_default, map->nTiles);

    n = -(cacheCode / 10);
    n = G3D_MAX(1, n);
    cacheCode = -((-cacheCode) % 10);

    if (cacheCode == G3D_USE_CACHE_X)
	size = map->nx * n;
    else if (cacheCode == G3D_USE_CACHE_Y)
	size = map->ny * n;
    else if (cacheCode == G3D_USE_CACHE_Z)
	size = map->nz * n;
    else if (cacheCode == G3D_USE_CACHE_XY)
	size = map->nxy * n;
    else if (cacheCode == G3D_USE_CACHE_XZ)
	size = map->nx * map->nz * n;
    else if (cacheCode == G3D_USE_CACHE_YZ)
	size = map->ny * map->nz * n;
    else if (cacheCode == G3D_USE_CACHE_XYZ)
	size = map->nTiles;
    else
	G3d_fatalError("G3d__computeCacheSize: invalid cache code");

    return G3D_MIN(size, map->nTiles);
}

/*---------------------------------------------------------------------------*/

/* this function does actually more than filling the header fields of the */
/* G3D-Map structure. It also allocates memory for compression and xdr, */
/* and initializes the index and cache. This function should be taken apart. */

int
G3d_fillHeader(G3D_Map * map, int operation, int compression, int useRle,
	       int useLzw, int type, int precision, int cache, int hasIndex,
	       int useXdr, int typeIntern, int nofHeaderBytes, int tileX,
	       int tileY, int tileZ, int proj, int zone, double north,
	       double south, double east, double west, double top,
	       double bottom, int rows, int cols, int depths, double ew_res,
	       double ns_res, double tb_res, char *unit)
{
    if (!G3D_VALID_OPERATION(operation))
	G3d_fatalError("G3d_fillHeader: operation not valid\n");

    map->operation = operation;

    map->unit = G_store(unit);

    map->region.proj = proj;
    map->region.zone = zone;

    map->region.north = north;
    map->region.south = south;
    map->region.east = east;
    map->region.west = west;
    map->region.top = top;
    map->region.bottom = bottom;

    map->region.rows = rows;
    map->region.cols = cols;
    map->region.depths = depths;

    map->region.ew_res = ew_res;
    map->region.ns_res = ns_res;
    map->region.tb_res = tb_res;

    G3d_adjustRegion(&(map->region));

    map->tileX = tileX;
    map->tileY = tileY;
    map->tileZ = tileZ;
    map->tileXY = map->tileX * map->tileY;
    map->tileSize = map->tileXY * map->tileZ;

    map->nx = (map->region.cols - 1) / tileX + 1;
    map->ny = (map->region.rows - 1) / tileY + 1;
    map->nz = (map->region.depths - 1) / tileZ + 1;
    map->nxy = map->nx * map->ny;
    map->nTiles = map->nxy * map->nz;

    if ((map->region.cols) % map->tileX != 0)
	map->clipX = map->nx - 1;
    else
	map->clipX = -1;
    if ((map->region.rows) % map->tileY != 0)
	map->clipY = map->ny - 1;
    else
	map->clipY = -1;
    if ((map->region.depths) % map->tileZ != 0)
	map->clipZ = map->nz - 1;
    else
	map->clipZ = -1;

    if ((type != FCELL_TYPE) && (type != DCELL_TYPE))
	G3d_fatalError("G3d_fillHeader: invalid type");
    map->type = type;

    if ((typeIntern != FCELL_TYPE) && (typeIntern != DCELL_TYPE))
	G3d_fatalError("G3d_fillHeader: invalid type");
    map->typeIntern = typeIntern;

    if (!G3D_VALID_XDR_OPTION(useXdr))
	G3d_fatalError("G3d_fillHeader: invalid xdr option");
    map->useXdr = useXdr;

    map->offset = nofHeaderBytes;

    if ((map->fileEndPtr = lseek(map->data_fd, (long)0, SEEK_END)) == -1) {
	G3d_error("G3d_fillHeader: can't position file");
	return 0;
    }

    map->useCache = (cache != G3D_NO_CACHE);

    map->numLengthIntern = G3d_length(map->typeIntern);
    map->numLengthExtern = G3d_externLength(map->type);

    map->compression = compression;
    map->useRle = useRle;
    map->useLzw = useLzw;
    map->precision = precision;

#define RLE_STATUS_BYTES 2

    if (map->compression != G3D_NO_COMPRESSION) {
	if (tmpCompress == NULL) {
	    tmpCompressLength = map->tileSize *
		G3D_MAX(map->numLengthIntern, map->numLengthExtern) +
		RLE_STATUS_BYTES;
	    tmpCompress = G3d_malloc(tmpCompressLength);
	    if (tmpCompress == NULL) {
		G3d_error("G3d_fillHeader: error in G3d_malloc");
		return 0;
	    }
	}
	else if (map->tileSize *
		 G3D_MAX(map->numLengthIntern, map->numLengthExtern)
		 + RLE_STATUS_BYTES > tmpCompressLength) {
	    tmpCompressLength = map->tileSize *
		G3D_MAX(map->numLengthIntern, map->numLengthExtern) +
		RLE_STATUS_BYTES;
	    tmpCompress = G3d_realloc(tmpCompress, tmpCompressLength);
	    if (tmpCompress == NULL) {
		G3d_error("G3d_fillHeader: error in G3d_realloc");
		return 0;
	    }
	}
    }

#define XDR_MISUSE_BYTES 10

    if (!G3d_initFpXdr(map, XDR_MISUSE_BYTES)) {
	G3d_error("G3d_fillHeader: error in G3d_initFpXdr");
	return 0;
    }

    if ((!map->useCache) ||
	((cache == G3D_USE_CACHE_DEFAULT) && (g3d_cache_default == 0))) {
	map->useCache = 0;
	map->cache = NULL;
	/* allocate one tile buffer */
	map->data = G3d_malloc(map->tileSize * map->numLengthIntern);
	if (map->data == NULL) {
	    G3d_error("G3d_fillHeader: error in G3d_malloc");
	    return 0;
	}
	map->currentIndex = -1;
    }
    else {
	if (!G3d_initCache(map,
			   G3D_MAX(1,
				   G3D_MIN(G3d__computeCacheSize(map, cache),
					   g3d_cache_max /
					   map->tileSize /
					   map->numLengthIntern)))) {
	    G3d_error("G3d_fillHeader: error in G3d_initCache");
	    return 0;
	}
    }

    if (!G3d_initIndex(map, hasIndex)) {
	G3d_error("G3d_fillHeader: error in G3d_initIndex");
	return 0;
    }

    return 1;
}
