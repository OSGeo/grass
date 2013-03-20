/*!
  \file lib/raster3d/close.c
  
  \brief 3D Raster Library - Close 3D raster file
 
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author USACERL and many others
*/

#ifdef __MINGW32__
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "raster3d_intern.h"

static int close_new(RASTER3D_Map * map)
{
    char path[GPATH_MAX];
    struct Categories cats;
    struct History hist;

    Rast3d_remove_color(map->fileName);

    /* create empty cats file */
    Rast_init_cats(NULL, &cats);
    Rast3d_write_cats(map->fileName, &cats);
    Rast_free_cats(&cats);

    /*genrate the history file, use the normal G_ functions */
    Rast_short_history(map->fileName, "raster3d", &hist);
    Rast_command_history(&hist);
    /*Use the G3d function to write the history file,
     * otherwise the path is wrong */
    if (Rast3d_write_history(map->fileName, &hist) < 0) {
	G_warning(_("Unable to write history for 3D raster map <%s>"), map->fileName);
    }

    Rast3d_range_write(map);

    close(map->data_fd);

    /* finally move tempfile to data file */
    Rast3d_filename(path, RASTER3D_CELL_ELEMENT, map->fileName, map->mapset);
#ifdef __MINGW32__
    if (CopyFile(map->tempName, path, FALSE) == 0) {
#else
    if (link(map->tempName, path) < 0) {
#endif
	if (rename(map->tempName, path)) {
	    G_warning(_("Unable to move temp raster map <%s> to 3d data file <%s>"),
		      map->tempName, path);
	    return 0;
	}
    }
    else
	remove(map->tempName);

    return 1;
}

static int close_cell_new(RASTER3D_Map * map)
{
    long ltmp;

    if (map->useCache)
	if (!Rast3d_flush_all_tiles(map)) {
	    G_warning(_("Unable to flush all tiles"));
	    return 0;
	}

    if (!Rast3d_flush_index(map)) {
	G_warning(_("Unable to flush index"));
	return 0;
    }

    /* write the header info which was filled with dummy values at the */
    /* opening time */

    if (lseek(map->data_fd,
	      (long)(map->offset - sizeof(int) - sizeof(long)),
	      SEEK_SET) == -1) {
	G_warning(_("Unable to position file"));
	return 0;
    }

    if (!Rast3d_write_ints(map->data_fd, map->useXdr, &(map->indexNbytesUsed), 1)) {
	G_warning(_("Unable to write header for 3D raster map <%s>"), map->fileName);
	return 0;
    }

    Rast3d_long_encode(&(map->indexOffset), (unsigned char *)&ltmp, 1);
    if (write(map->data_fd, &ltmp, sizeof(long)) != sizeof(long)) {
	G_warning(_("Unable to write header for 3D raster map <%s>"), map->fileName);
	return 0;
    }

    if (!close_new(map) != 0) {
	G_warning(_("Unable to create 3D raster map <%s>"), map->fileName);
	return 0;
    }

    return 1;
}

static int close_old(RASTER3D_Map * map)
{
    if (close(map->data_fd) != 0) {
	G_warning(_("Unable to close 3D raster map <%s>"), map->fileName);
	return 0;
    }

    return 1;
}

static int close_cell_old(RASTER3D_Map * map)
{
    if (!close_old(map) != 0) {
	G_warning(_("Unable to close 3D raster map <%s>"), map->fileName);
	return 0;
    }

    return 1;
}

/*!
  \brief Close 3D raster map files
  
  Closes g3d-file. If <em>map</em> is new and cache-mode is used for
  <em>map</em> then every tile which is not flushed before closing is
  flushed.
  
  \param map pointer to RASTER3D_Map to be closed

  \return 1 success
  \return 0 failure
*/
int Rast3d_close(RASTER3D_Map * map)
{
    if (map->operation == RASTER3D_WRITE_DATA) {
	if (!close_cell_new(map)) {
	    G_warning(_("Unable to create 3D raster map <%s>"), map->fileName);
	    return 0;
	}
    }
    else {
	if (!close_cell_old(map) != 0) {
	    G_warning(_("Unable to close 3D raster map <%s>"), map->fileName);
	    return 0;
	}
    }

    Rast3d_free(map->index);
    Rast3d_free(map->tileLength);

    if (map->useCache) {
	if (!Rast3d_dispose_cache(map)) {
	    G_warning(_("Error in cache"));
	    return 0;
	}
    }
    else
	Rast3d_free(map->data);

    if (map->operation == RASTER3D_WRITE_DATA)
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
			     map->hasIndex, map->unit, map->vertical_unit, map->version)) {
	    G_warning(_("Unable to write header for 3D raster map <%s>"), map->fileName);
	    return 0;
	}

    Rast3d_free(map);
    
    return 1;
}
