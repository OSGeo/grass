#ifdef __MINGW32__
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/raster.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

static int G3d_closeNew(RASTER3D_Map * map)
{
    char path[GPATH_MAX];
    struct Categories cats;
    struct History hist;

    G3d_removeColor(map->fileName);

    /* create empty cats file */
    Rast_init_cats(NULL, &cats);
    G3d_writeCats(map->fileName, &cats);
    Rast_free_cats(&cats);

    /*genrate the history file, use the normal G_ functions */
    Rast_short_history(map->fileName, "raster3d", &hist);
    Rast_command_history(&hist);
    /*Use the G3d function to write the history file,
     * otherwise the path is wrong */
    if (!G3d_writeHistory(map->fileName, &hist)) {
	G3d_error("G3d_closeNew: can't write raster3d history");
    }


    G3d_range_write(map);

    close(map->data_fd);

    /* finally move tempfile to data file */
    G3d_filename(path, RASTER3D_CELL_ELEMENT, map->fileName, map->mapset);
#ifdef __MINGW32__
    if (CopyFile(map->tempName, path, FALSE) == 0) {
#else
    if (link(map->tempName, path) < 0) {
#endif
	if (rename(map->tempName, path)) {
	    G3d_error
		("G3d_closeNew: can't move temp raster map %s\nto 3d data file %s",
		 map->tempName, path);
	    return 0;
	}
    }
    else
	remove(map->tempName);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_closeCellNew(RASTER3D_Map * map)
{
    long ltmp;

    if (map->useCache)
	if (!G3d_flushAllTiles(map)) {
	    G3d_error("G3d_closeCellNew: error in G3d_flushAllTiles");
	    return 0;
	}

    if (!G3d_flushIndex(map)) {
	G3d_error("G3d_closeCellNew: error in G3d_flushIndex");
	return 0;
    }

    /* write the header info which was filled with dummy values at the */
    /* opening time */

    if (lseek(map->data_fd,
	      (long)(map->offset - sizeof(int) - sizeof(long)),
	      SEEK_SET) == -1) {
	G3d_error("G3d_closeCellNew: can't position file");
	return 0;
    }

    if (!G3d_writeInts(map->data_fd, map->useXdr, &(map->indexNbytesUsed), 1)) {
	G3d_error("G3d_closeCellNew: can't write header");
	return 0;
    }

    G3d_longEncode(&(map->indexOffset), (unsigned char *)&ltmp, 1);
    if (write(map->data_fd, &ltmp, sizeof(long)) != sizeof(long)) {
	G3d_error("G3d_closeCellNew: can't write header");
	return 0;
    }

    if (!G3d_closeNew(map) != 0) {
	G3d_error("G3d_closeCellNew: error in G3d_closeNew");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_closeOld(RASTER3D_Map * map)
{
    if (close(map->data_fd) != 0) {
	G3d_error("G3d_closeOld: could not close file");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int G3d_closeCellOld(RASTER3D_Map * map)
{
    if (!G3d_closeOld(map) != 0) {
	G3d_error("G3d_closeCellOld: error in G3d_closeOld");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Closes g3d-file. If <em>map</em> is new
 * and cache-mode is used for <em>map</em> then every tile which is not flushed
 * before closing is flushed.  
 *
 *  \param map
 *  \return 1 ... if successful,
 *          0 ...  otherwise.
 */

int G3d_closeCell(RASTER3D_Map * map)
{
    if (map->operation == RASTER3D_WRITE_DATA) {
	if (!G3d_closeCellNew(map)) {
	    G3d_error("G3d_closeCell: error in G3d_closeCellNew");
	    return 0;
	}
    }
    else {
	if (!G3d_closeCellOld(map) != 0) {
	    G3d_error("G3d_closeCell: error in G3d_closeCellOld");
	    return 0;
	}
    }

    G3d_free(map->index);
    G3d_free(map->tileLength);

    if (map->useCache) {
	if (!G3d_disposeCache(map)) {
	    G3d_error("G3d_closeCell: error in G3d_disposeCache");
	    return 0;
	}
    }
    else
	G3d_free(map->data);

    if (map->operation == RASTER3D_WRITE_DATA)
	if (!G3d_writeHeader(map,
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
			     map->hasIndex, map->unit)) {
	    G3d_error("G3d_closeCell: error in G3d_writeHeader");
	    return 0;
	}

    G3d_free(map->unit);
    G3d_free(map);
    return 1;
}
