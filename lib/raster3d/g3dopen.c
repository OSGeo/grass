#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/G3d.h>
#include <grass/glocale.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

void *G3d_openCellOldNoHeader(const char *name, const char *mapset)
{
    G3D_Map *map;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G3d_initDefaults();

    if (!G3d_maskOpenOld()) {
	G3d_error(_("G3d_openCellOldNoHeader: error in G3d_maskOpenOld"));
	return (void *)NULL;
    }

    map = G3d_malloc(sizeof(G3D_Map));
    if (map == NULL) {
	G3d_error(_("G3d_openCellOldNoHeader: error in G3d_malloc"));
	return (void *)NULL;
    }

    G_unqualified_name(name, mapset, xname, xmapset);

    map->fileName = G_store(xname);
    map->mapset = G_store(xmapset);

    map->data_fd = G_open_old_misc(G3D_DIRECTORY, G3D_CELL_ELEMENT, xname, xmapset);
    if (map->data_fd < 0) {
	G3d_error(_("G3d_openCellOldNoHeader: error in G_open_old"));
	return (void *)NULL;
    }

    G3d_range_init(map);
    G3d_maskOff(map);

    return map;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Opens existing g3d-file <em>name</em> in <em>mapset</em>.
 * Tiles are stored in memory with <em>type</em> which must be any of FCELL_TYPE,
 * DCELL_TYPE, or G3D_TILE_SAME_AS_FILE. <em>cache</em> specifies the
 * cache-mode used and must be either G3D_NO_CACHE, G3D_USE_CACHE_DEFAULT,
 * G3D_USE_CACHE_X, G3D_USE_CACHE_Y, G3D_USE_CACHE_Z,
 * G3D_USE_CACHE_XY, G3D_USE_CACHE_XZ, G3D_USE_CACHE_YZ,
 * G3D_USE_CACHE_XYZ, the result of <tt>G3d_cacheSizeEncode ()</tt> (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer which
 * specifies the number of tiles buffered in the cache.  <em>window</em> sets the
 * window-region for the map. It is either a pointer to a window structure or
 * G3D_DEFAULT_WINDOW, which uses the window stored at initialization time or
 * set via <tt>G3d_setWindow ()</tt> (cf.{g3d:G3d.setWindow}).
 * To modify the window for the map after it has already been opened use
 * <tt>G3d_setWindowMap ()</tt> (cf.{g3d:G3d.setWindowMap}).
 * Returns a pointer to the cell structure ... if successful, NULL ...
 * otherwise.
 *
 *  \param name
 *  \param mapset
 *  \param window
 *  \param type
 *  \param cache
 *  \return void * 
 */

void *G3d_openCellOld(const char *name, const char *mapset,
		      G3D_Region * window, int typeIntern, int cache)
{
    G3D_Map *map;
    int proj, zone;
    int compression, useRle, useLzw, type, tileX, tileY, tileZ;
    int rows, cols, depths, precision;
    double ew_res, ns_res, tb_res;
    int nofHeaderBytes, dataOffset, useXdr, hasIndex;
    char *ltmp, *unit;
    double north, south, east, west, top, bottom;

    map = G3d_openCellOldNoHeader(name, mapset);
    if (map == NULL) {
	G3d_error(_("G3d_openCellOld: error in G3d_openCellOldNoHeader"));
	return (void *)NULL;
    }

    if (lseek(map->data_fd, (long)0, SEEK_SET) == -1) {
	G3d_error(_("G3d_openCellOld: can't rewind file"));
	return (void *)NULL;
    }

    if (!G3d_readHeader(map,
			&proj, &zone,
			&north, &south, &east, &west, &top, &bottom,
			&rows, &cols, &depths,
			&ew_res, &ns_res, &tb_res,
			&tileX, &tileY, &tileZ,
			&type, &compression, &useRle, &useLzw,
			&precision, &dataOffset, &useXdr, &hasIndex, &unit)) {
	G3d_error(_("G3d_openCellOld: error in G3d_readHeader"));
	return 0;
    }

    if (window == G3D_DEFAULT_WINDOW)
	window = G3d_windowPtr();

    if (proj != window->proj) {
	G3d_error(_("G3d_openCellOld: projection does not match window projection"));
	return (void *)NULL;
    }
    if (zone != window->zone) {
	G3d_error(_("G3d_openCellOld: zone does not match window zone"));
	return (void *)NULL;
    }

    map->useXdr = useXdr;

    if (hasIndex) {
	/* see G3D_openCell_new () for format of header */
	if ((!G3d_readInts(map->data_fd, map->useXdr,
			   &(map->indexLongNbytes), 1)) ||
	    (!G3d_readInts(map->data_fd, map->useXdr,
			   &(map->indexNbytesUsed), 1))) {
	    G3d_error(_("G3d_openCellOld: can't read header"));
	    return (void *)NULL;
	}

	/* if our long is to short to store offsets we can't read the file */
	if (map->indexNbytesUsed > sizeof(long))
	    G3d_fatalError(_("G3d_openCellOld: index does not fit into long"));

	ltmp = G3d_malloc(map->indexLongNbytes);
	if (ltmp == NULL) {
	    G3d_error(_("G3d_openCellOld: error in G3d_malloc"));
	    return (void *)NULL;
	}

	/* convert file long to long */
	if (read(map->data_fd, ltmp, map->indexLongNbytes) !=
	    map->indexLongNbytes) {
	    G3d_error(_("G3d_openCellOld: can't read header"));
	    return (void *)NULL;
	}
	G3d_longDecode(ltmp, &(map->indexOffset), 1, map->indexLongNbytes);
	G3d_free(ltmp);
    }

    nofHeaderBytes = dataOffset;

    if (typeIntern == G3D_TILE_SAME_AS_FILE)
	typeIntern = type;

    if (!G3d_fillHeader(map, G3D_READ_DATA, compression, useRle, useLzw,
			type, precision, cache,
			hasIndex, map->useXdr, typeIntern,
			nofHeaderBytes, tileX, tileY, tileZ,
			proj, zone,
			north, south, east, west, top, bottom,
			rows, cols, depths, ew_res, ns_res, tb_res, unit)) {
	G3d_error(_("G3d_openCellOld: error in G3d_fillHeader"));
	return (void *)NULL;
    }

    G3d_regionCopy(&(map->window), window);
    G3d_adjustRegion(&(map->window));
    G3d_getNearestNeighborFunPtr(&(map->resampleFun));

    return map;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Opens new g3d-file with <em>name</em> in the current mapset. Tiles
 * are stored in memory with <em>type</em> which must be one of FCELL_TYPE,
 * DCELL_TYPE, or G3D_TILE_SAME_AS_FILE. <em>cache</em> specifies the
 * cache-mode used and must be either G3D_NO_CACHE, G3D_USE_CACHE_DEFAULT,
 * G3D_USE_CACHE_X, G3D_USE_CACHE_Y, G3D_USE_CACHE_Z,
 * G3D_USE_CACHE_XY, G3D_USE_CACHE_XZ, G3D_USE_CACHE_YZ,
 * G3D_USE_CACHE_XYZ, the result of <tt>G3d_cacheSizeEncode ()</tt>
 * (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer which
 * specifies the number of tiles buffered in the cache.  <em>region</em> specifies
 * the 3d region.  
 * Returns a pointer to the cell structure ... if successful,
 * NULL ... otherwise.
 *
 *  \param name
 *  \param type
 *  \param cache
 *  \param region
 *  \return void * 
 */

void *G3d_openCellNew(const char *name, int typeIntern, int cache,
		      G3D_Region * region)
{
    G3D_Map *map;
    int nofHeaderBytes, dummy = 0, compression, precision;
    long ldummy = 0;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G3d_initDefaults();
    if (!G3d_maskOpenOld()) {
	G3d_error(_("G3d_openCellNew: error in G3d_maskOpenOld"));
	return (void *)NULL;
    }

    compression = g3d_do_compression;
    precision = g3d_precision;

    map = G3d_malloc(sizeof(G3D_Map));
    if (map == NULL) {
	G3d_error(_("G3d_openCellNew: error in G3d_malloc"));
	return (void *)NULL;
    }

    if (G_unqualified_name(name, G_mapset(), xname, xmapset) < 0) {
	G_warning(_("map <%s> is not in the current mapset"), name);
	return (void *)NULL;
    }

    map->fileName = G_store(xname);
    map->mapset = G_store(xmapset);

    map->tempName = G_tempfile();
    map->data_fd = open(map->tempName, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (map->data_fd < 0) {
	G3d_error(_("G3d_openCellNew: could not open file"));
	return (void *)NULL;
    }

    G3d_makeMapsetMapDirectory(map->fileName);

    map->useXdr = G3D_USE_XDR;

    if (g3d_file_type == FCELL_TYPE) {
	if (precision > 23)
	    precision = 23;	/* 32 - 8 - 1 */
	else if (precision < -1)
	    precision = 0;
    }
    else if (precision > 52)
	precision = 52;		/* 64 - 11 - 1 */
    else if (precision < -1)
	precision = 0;

    /* no need to write trailing zeros */
    if ((typeIntern == FCELL_TYPE) && (g3d_file_type == DCELL_TYPE)) {
	if (precision == -1)
	    precision = 23;
	else
	    precision = G3D_MIN(precision, 23);
    }

    if (compression == G3D_NO_COMPRESSION)
	precision = G3D_MAX_PRECISION;
    if (compression == G3D_COMPRESSION)
	map->useXdr = G3D_USE_XDR;

    if (G3D_HAS_INDEX) {
	map->indexLongNbytes = sizeof(long);

	/* at the beginning of the file write */
	/*      nof bytes of "long" */
	/*      max nof bytes used for index */
	/*      position of index in file */
	/* the index is appended at the end of the file at closing time. since */
	/* we do not know this position yet we write dummy values */

	if ((!G3d_writeInts(map->data_fd, map->useXdr,
			    &(map->indexLongNbytes), 1)) ||
	    (!G3d_writeInts(map->data_fd, map->useXdr, &dummy, 1))) {
	    G3d_error(_("G3d_openCellNew: can't write header"));
	    return (void *)NULL;
	}
	if (write(map->data_fd, &ldummy, map->indexLongNbytes) !=
	    map->indexLongNbytes) {
	    G3d_error(_("G3d_openCellNew: can't write header"));
	    return (void *)NULL;
	}
    }

    /* can't use a constant since this depends on sizeof (long) */
    nofHeaderBytes = lseek(map->data_fd, (long)0, SEEK_CUR);

    G3d_range_init(map);
    G3d_adjustRegion(region);

    if (!G3d_fillHeader(map, G3D_WRITE_DATA, compression,
			g3d_do_rle_compression, g3d_do_lzw_compression,
			g3d_file_type, precision, cache, G3D_HAS_INDEX,
			map->useXdr, typeIntern, nofHeaderBytes,
			g3d_tile_dimension[0], g3d_tile_dimension[1],
			g3d_tile_dimension[2],
			region->proj, region->zone,
			region->north, region->south, region->east,
			region->west, region->top, region->bottom,
			region->rows, region->cols, region->depths,
			region->ew_res, region->ns_res, region->tb_res,
			g3d_unit_default)) {
	G3d_error(_("G3d_openCellNew: error in G3d_fillHeader"));
	return (void *)NULL;
    }

    /*Set the map window to the map region */
    G3d_regionCopy(&(map->window), region);
    /*Set the resampling function to nearest neighbor for data access */
    G3d_getNearestNeighborFunPtr(&(map->resampleFun));

    G3d_maskOff(map);

    return (void *)map;
}
