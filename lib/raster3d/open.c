#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

void *Rast3d_open_cell_old_no_header(const char *name, const char *mapset)
{
    RASTER3D_Map *map;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    Rast3d_init_defaults();

    if (!Rast3d_mask_open_old()) {
	Rast3d_error(_("Rast3d_open_cell_old_no_header: error in Rast3d_mask_open_old"));
	return (void *)NULL;
    }

    map = Rast3d_malloc(sizeof(RASTER3D_Map));
    if (map == NULL) {
	Rast3d_error(_("Rast3d_open_cell_old_no_header: error in Rast3d_malloc"));
	return (void *)NULL;
    }

    G_unqualified_name(name, mapset, xname, xmapset);

    map->fileName = G_store(xname);
    map->mapset = G_store(xmapset);

    map->data_fd = G_open_old_misc(RASTER3D_DIRECTORY, RASTER3D_CELL_ELEMENT, xname, xmapset);
    if (map->data_fd < 0) {
	Rast3d_error(_("Rast3d_open_cell_old_no_header: error in G_open_old"));
	return (void *)NULL;
    }

    Rast3d_range_init(map);
    Rast3d_mask_off(map);

    return map;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Opens existing g3d-file <em>name</em> in <em>mapset</em>.
 * Tiles are stored in memory with <em>type</em> which must be any of FCELL_TYPE,
 * DCELL_TYPE, or RASTER3D_TILE_SAME_AS_FILE. <em>cache</em> specifies the
 * cache-mode used and must be either RASTER3D_NO_CACHE, RASTER3D_USE_CACHE_DEFAULT,
 * RASTER3D_USE_CACHE_X, RASTER3D_USE_CACHE_Y, RASTER3D_USE_CACHE_Z,
 * RASTER3D_USE_CACHE_XY, RASTER3D_USE_CACHE_XZ, RASTER3D_USE_CACHE_YZ,
 * RASTER3D_USE_CACHE_XYZ, the result of <tt>Rast3d_cache_size_encode ()</tt> (cf.{g3d:G3d.cacheSizeEncode}), or any positive integer which
 * specifies the number of tiles buffered in the cache.  <em>window</em> sets the
 * window-region for the map. It is either a pointer to a window structure or
 * RASTER3D_DEFAULT_WINDOW, which uses the window stored at initialization time or
 * set via <tt>Rast3d_set_window ()</tt> (cf.{g3d:G3d.setWindow}).
 * To modify the window for the map after it has already been opened use
 * <tt>Rast3d_set_window_map ()</tt> (cf.{g3d:G3d.setWindowMap}).
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

void *Rast3d_open_cell_old(const char *name, const char *mapset,
		      RASTER3D_Region * window, int typeIntern, int cache)
{
    RASTER3D_Map *map;
    int proj, zone;
    int compression, useRle, useLzw, type, tileX, tileY, tileZ;
    int rows, cols, depths, precision;
    double ew_res, ns_res, tb_res;
    int nofHeaderBytes, dataOffset, useXdr, hasIndex;
    char *ltmp, *unit;
    int vertical_unit;
    int version;
    double north, south, east, west, top, bottom;

    map = Rast3d_open_cell_old_no_header(name, mapset);
    if (map == NULL) {
	Rast3d_error(_("Rast3d_open_cell_old: error in Rast3d_open_cell_old_no_header"));
	return (void *)NULL;
    }

    if (lseek(map->data_fd, (long)0, SEEK_SET) == -1) {
	Rast3d_error(_("Rast3d_open_cell_old: can't rewind file"));
	return (void *)NULL;
    }

    if (!Rast3d_read_header(map,
			&proj, &zone,
			&north, &south, &east, &west, &top, &bottom,
			&rows, &cols, &depths,
			&ew_res, &ns_res, &tb_res,
			&tileX, &tileY, &tileZ,
			&type, &compression, &useRle, &useLzw,
			&precision, &dataOffset, &useXdr, &hasIndex, &unit, &vertical_unit,
			&version)) {
	Rast3d_error(_("Rast3d_open_cell_old: error in Rast3d_read_header"));
	return 0;
    }

    if (window == RASTER3D_DEFAULT_WINDOW)
	window = Rast3d_window_ptr();

    if (proj != window->proj) {
	Rast3d_error(_("Rast3d_open_cell_old: projection does not match window projection"));
	return (void *)NULL;
    }
    if (zone != window->zone) {
	Rast3d_error(_("Rast3d_open_cell_old: zone does not match window zone"));
	return (void *)NULL;
    }

    map->useXdr = useXdr;

    if (hasIndex) {
	/* see RASTER3D_openCell_new () for format of header */
	if ((!Rast3d_read_ints(map->data_fd, map->useXdr,
			   &(map->indexLongNbytes), 1)) ||
	    (!Rast3d_read_ints(map->data_fd, map->useXdr,
			   &(map->indexNbytesUsed), 1))) {
	    Rast3d_error(_("Rast3d_open_cell_old: can't read header"));
	    return (void *)NULL;
	}

	/* if our long is to short to store offsets we can't read the file */
	if (map->indexNbytesUsed > sizeof(long))
	    Rast3d_fatal_error(_("Rast3d_open_cell_old: index does not fit into long"));

	ltmp = Rast3d_malloc(map->indexLongNbytes);
	if (ltmp == NULL) {
	    Rast3d_error(_("Rast3d_open_cell_old: error in Rast3d_malloc"));
	    return (void *)NULL;
	}

	/* convert file long to long */
	if (read(map->data_fd, ltmp, map->indexLongNbytes) !=
	    map->indexLongNbytes) {
	    Rast3d_error(_("Rast3d_open_cell_old: can't read header"));
	    return (void *)NULL;
	}
	Rast3d_long_decode(ltmp, &(map->indexOffset), 1, map->indexLongNbytes);
	Rast3d_free(ltmp);
    }

    nofHeaderBytes = dataOffset;

    if (typeIntern == RASTER3D_TILE_SAME_AS_FILE)
	typeIntern = type;

    if (!Rast3d_fill_header(map, RASTER3D_READ_DATA, compression, useRle, useLzw,
			type, precision, cache,
			hasIndex, map->useXdr, typeIntern,
			nofHeaderBytes, tileX, tileY, tileZ,
			proj, zone,
			north, south, east, west, top, bottom,
			rows, cols, depths, ew_res, ns_res, tb_res, unit, vertical_unit,
			version)) {
	Rast3d_error(_("Rast3d_open_cell_old: error in Rast3d_fill_header"));
	return (void *)NULL;
    }

    Rast3d_region_copy(&(map->window), window);
    Rast3d_adjust_region(&(map->window));
    Rast3d_get_nearest_neighbor_fun_ptr(&(map->resampleFun));

    return map;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Opens new g3d-file with <em>name</em> in the current mapset. Tiles
 * are stored in memory with <em>type</em> which must be one of FCELL_TYPE,
 * DCELL_TYPE, or RASTER3D_TILE_SAME_AS_FILE. <em>cache</em> specifies the
 * cache-mode used and must be either RASTER3D_NO_CACHE, RASTER3D_USE_CACHE_DEFAULT,
 * RASTER3D_USE_CACHE_X, RASTER3D_USE_CACHE_Y, RASTER3D_USE_CACHE_Z,
 * RASTER3D_USE_CACHE_XY, RASTER3D_USE_CACHE_XZ, RASTER3D_USE_CACHE_YZ,
 * RASTER3D_USE_CACHE_XYZ, the result of <tt>Rast3d_cache_size_encode ()</tt>
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

void *Rast3d_open_cell_new(const char *name, int typeIntern, int cache,
		      RASTER3D_Region * region)
{
    RASTER3D_Map *map;
    int nofHeaderBytes, dummy = 0, compression, precision;
    long ldummy = 0;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    Rast3d_init_defaults();
    if (!Rast3d_mask_open_old()) {
	Rast3d_error(_("Rast3d_open_cell_new: error in Rast3d_mask_open_old"));
	return (void *)NULL;
    }

    compression = g3d_do_compression;
    precision = g3d_precision;

    map = Rast3d_malloc(sizeof(RASTER3D_Map));
    if (map == NULL) {
	Rast3d_error(_("Rast3d_open_cell_new: error in Rast3d_malloc"));
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
	Rast3d_error(_("Rast3d_open_cell_new: could not open file"));
	return (void *)NULL;
    }

    Rast3d_make_mapset_map_directory(map->fileName);

    /* XDR support has been removed */
    map->useXdr = RASTER3D_NO_XDR;

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
	    precision = RASTER3D_MIN(precision, 23);
    }

    if (compression == RASTER3D_NO_COMPRESSION)
	precision = RASTER3D_MAX_PRECISION;

    if (RASTER3D_HAS_INDEX) {
	map->indexLongNbytes = sizeof(long);

	/* at the beginning of the file write */
	/*      nof bytes of "long" */
	/*      max nof bytes used for index */
	/*      position of index in file */
	/* the index is appended at the end of the file at closing time. since */
	/* we do not know this position yet we write dummy values */

	if ((!Rast3d_write_ints(map->data_fd, map->useXdr,
			    &(map->indexLongNbytes), 1)) ||
	    (!Rast3d_write_ints(map->data_fd, map->useXdr, &dummy, 1))) {
	    Rast3d_error(_("Rast3d_open_cell_new: can't write header"));
	    return (void *)NULL;
	}
	if (write(map->data_fd, &ldummy, map->indexLongNbytes) !=
	    map->indexLongNbytes) {
	    Rast3d_error(_("Rast3d_open_cell_new: can't write header"));
	    return (void *)NULL;
	}
    }

    /* can't use a constant since this depends on sizeof (long) */
    nofHeaderBytes = lseek(map->data_fd, (long)0, SEEK_CUR);

    Rast3d_range_init(map);
    Rast3d_adjust_region(region);

    if (!Rast3d_fill_header(map, RASTER3D_WRITE_DATA, compression, 0, 0,
			g3d_file_type, precision, cache, RASTER3D_HAS_INDEX,
			map->useXdr, typeIntern, nofHeaderBytes,
			g3d_tile_dimension[0], g3d_tile_dimension[1],
			g3d_tile_dimension[2],
			region->proj, region->zone,
			region->north, region->south, region->east,
			region->west, region->top, region->bottom,
			region->rows, region->cols, region->depths,
			region->ew_res, region->ns_res, region->tb_res,
			g3d_unit_default, g3d_vertical_unit_default, RASTER3D_MAP_VERSION)) {
	Rast3d_error(_("Rast3d_open_cell_new: error in Rast3d_fill_header"));
	return (void *)NULL;
    }

    /*Set the map window to the map region */
    Rast3d_region_copy(&(map->window), region);
    /*Set the resampling function to nearest neighbor for data access */
    Rast3d_get_nearest_neighbor_fun_ptr(&(map->resampleFun));

    Rast3d_mask_off(map);

    return (void *)map;
}
