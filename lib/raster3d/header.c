#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

void *tmpCompress;
int tmpCompressLength;
void *xdr;
int xdrLength;

/*---------------------------------------------------------------------------*/

#define RASTER3D_HEADER_TILEX "TileDimensionX"
#define RASTER3D_HEADER_TILEY "TileDimensionY"
#define RASTER3D_HEADER_TILEZ "TileDimensionZ"
#define RASTER3D_HEADER_TYPE "CellType"
#define RASTER3D_HEADER_COMPRESSION "useCompression"
#define RASTER3D_HEADER_USERLE "useRle"
#define RASTER3D_HEADER_USELZW "useLzw"
#define RASTER3D_HEADER_PRECISION "Precision"
#define RASTER3D_HEADER_DATA_OFFSET "nofHeaderBytes"
#define RASTER3D_HEADER_USEXDR "useXdr"
#define RASTER3D_HEADER_HASINDEX "hasIndex"
#define RASTER3D_HEADER_UNIT "Units"
#define RASTER3D_HEADER_VERTICAL_UNIT "VerticalUnits"
#define RASTER3D_HEADER_VERSION "Version"

/*---------------------------------------------------------------------------*/

static int
Rast3d_readWriteHeader(struct Key_Value *headerKeys, int doRead, int *proj,
		    int *zone, double *north, double *south, double *east,
		    double *west, double *top, double *bottom, int *rows,
		    int *cols, int *depths, double *ew_res, double *ns_res,
		    double *tb_res, int *tileX, int *tileY, int *tileZ,
		    int *type, int *compression, int *useRle, int *useLzw,
		    int *precision, int *dataOffset, int *useXdr,
		    int *hasIndex, char **unit, int *vertical_unit, int *version)
{
    int returnVal;
    int (*headerInt) (), (*headerDouble) (), (*headerValue) ();
    int (*headerString) ();

    if (doRead) {
	headerDouble = Rast3d_key_get_double;
	headerInt = Rast3d_key_get_int;
	headerString = Rast3d_key_get_string;
	headerValue = Rast3d_key_get_value;
    }
    else {
	headerDouble = Rast3d_key_set_double;
	headerInt = Rast3d_key_set_int;
	headerString = Rast3d_key_set_string;
	headerValue = Rast3d_key_set_value;
    }

    returnVal = 1;
    returnVal &= headerInt(headerKeys, RASTER3D_REGION_PROJ, proj);
    returnVal &= headerInt(headerKeys, RASTER3D_REGION_ZONE, zone);

    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_NORTH, north);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_SOUTH, south);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_EAST, east);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_WEST, west);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_TOP, top);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_BOTTOM, bottom);

    returnVal &= headerInt(headerKeys, RASTER3D_REGION_ROWS, rows);
    returnVal &= headerInt(headerKeys, RASTER3D_REGION_COLS, cols);
    returnVal &= headerInt(headerKeys, RASTER3D_REGION_DEPTHS, depths);

    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_NSRES, ns_res);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_EWRES, ew_res);
    returnVal &= headerDouble(headerKeys, RASTER3D_REGION_TBRES, tb_res);

    returnVal &= headerInt(headerKeys, RASTER3D_HEADER_TILEX, tileX);
    returnVal &= headerInt(headerKeys, RASTER3D_HEADER_TILEY, tileY);
    returnVal &= headerInt(headerKeys, RASTER3D_HEADER_TILEZ, tileZ);

    returnVal &= headerValue(headerKeys, RASTER3D_HEADER_TYPE,
			     "double", "float", DCELL_TYPE, FCELL_TYPE, type);
    returnVal &= headerValue(headerKeys, RASTER3D_HEADER_COMPRESSION,
			     "0", "1", 0, 1, compression);
    returnVal &= headerValue(headerKeys, RASTER3D_HEADER_USERLE,
			     "0", "1", 0, 1, useRle);
    returnVal &= headerValue(headerKeys, RASTER3D_HEADER_USELZW,
			     "0", "1", 0, 1, useLzw);

    returnVal &= headerInt(headerKeys, RASTER3D_HEADER_PRECISION, precision);
    returnVal &= headerInt(headerKeys, RASTER3D_HEADER_DATA_OFFSET, dataOffset);

    returnVal &= headerValue(headerKeys, RASTER3D_HEADER_USEXDR,
			     "0", "1", 0, 1, useXdr);
    returnVal &= headerValue(headerKeys, RASTER3D_HEADER_HASINDEX,
			     "0", "1", 0, 1, hasIndex);
    returnVal &= headerString(headerKeys, RASTER3D_HEADER_UNIT, unit);
    /* New format and API changes */
    if(!headerInt(headerKeys, RASTER3D_HEADER_VERTICAL_UNIT, vertical_unit))
        G_warning("You are using an old raster3d data format, the vertical unit is undefined. "
                  "Please use r3.support to define the vertical unit to avoid this warning.");
    /* New format and API changes */
    if(!headerInt(headerKeys, RASTER3D_HEADER_VERSION, version)) {
        G_warning("You are using an old raster3d data format, the version is undefined.");
        *version = 1;
    }

    if (returnVal)
	return 1;

    Rast3d_error("Rast3d_readWriteHeader: error reading/writing header");
    return 0;
}

/*---------------------------------------------------------------------------*/

int
Rast3d_read_header(RASTER3D_Map * map, int *proj, int *zone, double *north,
	       double *south, double *east, double *west, double *top,
	       double *bottom, int *rows, int *cols, int *depths,
	       double *ew_res, double *ns_res, double *tb_res, int *tileX,
	       int *tileY, int *tileZ, int *type, int *compression,
	       int *useRle, int *useLzw, int *precision, int *dataOffset,
	       int *useXdr, int *hasIndex, char **unit, int *vertical_unit,
	       int *version)
{
    struct Key_Value *headerKeys;
    char path[GPATH_MAX];

    Rast3d_filename(path, RASTER3D_HEADER_ELEMENT, map->fileName, map->mapset);
    if (access(path, R_OK) != 0) {
	Rast3d_error("Rast3d_read_header: unable to find [%s]", path);
	return 0;
    }

    headerKeys = G_read_key_value_file(path);

    if (!Rast3d_readWriteHeader(headerKeys, 1,
			     proj, zone,
			     north, south, east, west, top, bottom,
			     rows, cols, depths,
			     ew_res, ns_res, tb_res,
			     tileX, tileY, tileZ,
			     type, compression, useRle, useLzw, precision,
			     dataOffset, useXdr, hasIndex, unit, vertical_unit, version)) {
	Rast3d_error("Rast3d_read_header: error extracting header key(s) of file %s",
		  path);
	return 0;
    }

    G_free_key_value(headerKeys);
    return 1;
}

/*---------------------------------------------------------------------------*/

int
Rast3d_write_header(RASTER3D_Map * map, int proj, int zone, double north, double south,
		double east, double west, double top, double bottom, int rows,
		int cols, int depths, double ew_res, double ns_res,
		double tb_res, int tileX, int tileY, int tileZ, int type,
		int compression, int useRle, int useLzw, int precision,
		int dataOffset, int useXdr, int hasIndex, char *unit, int vertical_unit,
		int version)
{
    struct Key_Value *headerKeys;
    char path[GPATH_MAX];

    headerKeys = G_create_key_value();

    if (!Rast3d_readWriteHeader(headerKeys, 0,
			     &proj, &zone,
			     &north, &south, &east, &west, &top, &bottom,
			     &rows, &cols, &depths,
			     &ew_res, &ns_res, &tb_res,
			     &tileX, &tileY, &tileZ,
			     &type, &compression, &useRle, &useLzw,
			     &precision, &dataOffset, &useXdr, &hasIndex,
			     &unit, &vertical_unit, &version)) {
	Rast3d_error("Rast3d_write_header: error adding header key(s) for file %s",
		  path);
	return 0;
    }

    Rast3d_filename(path, RASTER3D_HEADER_ELEMENT, map->fileName, map->mapset);
    Rast3d_make_mapset_map_directory(map->fileName);
    G_write_key_value_file(path, headerKeys);

    G_free_key_value(headerKeys);

    return 1;
}
/*---------------------------------------------------------------------------*/

int
Rast3d_rewrite_header(RASTER3D_Map * map)
{
    if (!Rast3d_write_header(map,
                         map->region.proj, map->region.zone,
                         map->region.north, map->region.south,
                         map->region.east, map->region.west,
                         map->region.top, map->region.bottom,
                         map->region.rows, map->region.cols,
                         map->region.depths,
                         map->region.ew_res, map->region.ns_res,
                         map->region.tb_res,
                         map->tileX, map->tileY, map->tileZ,
                         map->type,
                         map->compression, map->useRle, map->useLzw,
                         map->precision, map->offset, map->useXdr,
                         map->hasIndex, map->unit, map->vertical_unit,
                         map->version)) {
        G_warning(_("Unable to write header for 3D raster map <%s>"), map->fileName);
        return 0;
    }
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
 * If <em>cacheCode</em> is RASTER3D_USE_CACHE_DEFAULT the function returns
 * RASTER3D_USE_CACHE_DEFAULT.
 * If <em>cacheCode</em> is RASTER3D_USE_CACHE_??? the function returns a value
 * encoding RASTER3D_USE_CACHE_??? and <em>n</em>. Here RASTER3D_USE_CACHE_??? is one
 * of RASTER3D_USE_CACHE_X, RASTER3D_USE_CACHE_Y, RASTER3D_USE_CACHE_Z,
 * RASTER3D_USE_CACHE_XY, RASTER3D_USE_CACHE_XZ, RASTER3D_USE_CACHE_YZ, or
 * RASTER3D_USE_CACHE_XYZ, where e.g.  RASTER3D_USE_CACHE_X specifies that the cache
 * should store as many tiles as there exist in one row along the x-axis of the
 * tile cube, and RASTER3D_USE_CACHE_XY specifies that the cache should store as
 * many tiles as there exist in one slice of the tile cube with constant Z
 * coordinate.
 *
 *  \param cacheCode
 *  \param n
 *  \return int
 */

int Rast3d_cache_size_encode(int cacheCode, int n)
{
    if (cacheCode >= RASTER3D_NO_CACHE)
	return cacheCode * n;
    if (cacheCode == RASTER3D_USE_CACHE_DEFAULT)
	return cacheCode;

    if (cacheCode < RASTER3D_USE_CACHE_XYZ)
	Rast3d_fatal_error("Rast3d_cache_size_encode: invalid cache code");

    return n * (-10) + cacheCode;
}

/*---------------------------------------------------------------------------*/

int Rast3d__compute_cache_size(RASTER3D_Map * map, int cacheCode)
{
    int n, size;

    if (cacheCode >= RASTER3D_NO_CACHE)
	return cacheCode;
    if (cacheCode == RASTER3D_USE_CACHE_DEFAULT)
	return RASTER3D_MIN(g3d_cache_default, map->nTiles);

    n = -(cacheCode / 10);
    n = RASTER3D_MAX(1, n);
    cacheCode = -((-cacheCode) % 10);

    if (cacheCode == RASTER3D_USE_CACHE_X)
	size = map->nx * n;
    else if (cacheCode == RASTER3D_USE_CACHE_Y)
	size = map->ny * n;
    else if (cacheCode == RASTER3D_USE_CACHE_Z)
	size = map->nz * n;
    else if (cacheCode == RASTER3D_USE_CACHE_XY)
	size = map->nxy * n;
    else if (cacheCode == RASTER3D_USE_CACHE_XZ)
	size = map->nx * map->nz * n;
    else if (cacheCode == RASTER3D_USE_CACHE_YZ)
	size = map->ny * map->nz * n;
    else if (cacheCode == RASTER3D_USE_CACHE_XYZ)
	size = map->nTiles;
    else
	Rast3d_fatal_error("Rast3d__compute_cache_size: invalid cache code");

    return RASTER3D_MIN(size, map->nTiles);
}

/*---------------------------------------------------------------------------*/

/* this function does actually more than filling the header fields of the */
/* RASTER3D-Map structure. It also allocates memory for compression and xdr, */
/* and initializes the index and cache. This function should be taken apart. */

int
Rast3d_fill_header(RASTER3D_Map * map, int operation, int compression, int useRle,
	       int useLzw, int type, int precision, int cache, int hasIndex,
	       int useXdr, int typeIntern, int nofHeaderBytes, int tileX,
	       int tileY, int tileZ, int proj, int zone, double north,
	       double south, double east, double west, double top,
	       double bottom, int rows, int cols, int depths, double ew_res,
	       double ns_res, double tb_res, char *unit, int vertical_unit,
	       int version)
{
    if (!RASTER3D_VALID_OPERATION(operation))
	Rast3d_fatal_error("Rast3d_fill_header: operation not valid\n");

    map->version = version;

    map->operation = operation;

    map->unit = G_store(unit);
    map->vertical_unit = vertical_unit;

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

    Rast3d_adjust_region(&(map->region));

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
	Rast3d_fatal_error("Rast3d_fill_header: invalid type");
    map->type = type;

    if ((typeIntern != FCELL_TYPE) && (typeIntern != DCELL_TYPE))
	Rast3d_fatal_error("Rast3d_fill_header: invalid type");
    map->typeIntern = typeIntern;

    if (!RASTER3D_VALID_XDR_OPTION(useXdr))
	Rast3d_fatal_error("Rast3d_fill_header: invalid xdr option");
    map->useXdr = useXdr; 	/* Only kept for backward compatibility */

    map->offset = nofHeaderBytes;

    if ((map->fileEndPtr = lseek(map->data_fd, (long)0, SEEK_END)) == -1) {
	Rast3d_error("Rast3d_fill_header: can't position file");
	return 0;
    }

    map->useCache = (cache != RASTER3D_NO_CACHE);

    map->numLengthIntern = Rast3d_length(map->typeIntern);
    map->numLengthExtern = Rast3d_extern_length(map->type);

    map->compression = compression;
    map->useRle = useRle;	/* Only kept for backward compatibility */
    map->useLzw = useLzw;	/* Only kept for backward compatibility */
    map->precision = precision;

#define RLE_STATUS_BYTES 2

    if (map->compression != RASTER3D_NO_COMPRESSION) {
	if (tmpCompress == NULL) {
	    tmpCompressLength = map->tileSize *
		RASTER3D_MAX(map->numLengthIntern, map->numLengthExtern) +
		RLE_STATUS_BYTES;
	    tmpCompress = Rast3d_malloc(tmpCompressLength);
	    if (tmpCompress == NULL) {
		Rast3d_error("Rast3d_fill_header: error in Rast3d_malloc");
		return 0;
	    }
	}
	else if (map->tileSize *
		 RASTER3D_MAX(map->numLengthIntern, map->numLengthExtern)
		 + RLE_STATUS_BYTES > tmpCompressLength) {
	    tmpCompressLength = map->tileSize *
		RASTER3D_MAX(map->numLengthIntern, map->numLengthExtern) +
		RLE_STATUS_BYTES;
	    tmpCompress = Rast3d_realloc(tmpCompress, tmpCompressLength);
	    if (tmpCompress == NULL) {
		Rast3d_error("Rast3d_fill_header: error in Rast3d_realloc");
		return 0;
	    }
	}
    }

#define XDR_MISUSE_BYTES 10

    if (!Rast3d_init_fp_xdr(map, XDR_MISUSE_BYTES)) {
	Rast3d_error("Rast3d_fill_header: error in Rast3d_init_fp_xdr");
	return 0;
    }

    if ((!map->useCache) ||
	((cache == RASTER3D_USE_CACHE_DEFAULT) && (g3d_cache_default == 0))) {
	map->useCache = 0;
	map->cache = NULL;
	/* allocate one tile buffer */
	map->data = Rast3d_malloc(map->tileSize * map->numLengthIntern);
	if (map->data == NULL) {
	    Rast3d_error("Rast3d_fill_header: error in Rast3d_malloc");
	    return 0;
	}
	map->currentIndex = -1;
    }
    else {
	if (!Rast3d_init_cache(map,
			   RASTER3D_MAX(1,
				   RASTER3D_MIN(Rast3d__compute_cache_size(map, cache),
					   g3d_cache_max /
					   map->tileSize /
					   map->numLengthIntern)))) {
	    Rast3d_error("Rast3d_fill_header: error in Rast3d_init_cache");
	    return 0;
	}
    }

    if (!Rast3d_init_index(map, hasIndex)) {
	Rast3d_error("Rast3d_fill_header: error in Rast3d_init_index");
	return 0;
    }

    return 1;
}
