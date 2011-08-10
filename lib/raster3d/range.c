#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

void
Rast3d_range_updateFromTile(RASTER3D_Map * map, const void *tile, int rows, int cols,
			 int depths, int xRedundant, int yRedundant,
			 int zRedundant, int nofNum, int type)
{
    int y, z, cellType;
    struct FPRange *range;

    range = &(map->range);
    cellType = Rast3d_g3dType2cellType(type);

    if (nofNum == map->tileSize) {
	Rast_row_update_fp_range(tile, map->tileSize, range, cellType);
	return;
    }

    if (xRedundant) {
	for (z = 0; z < depths; z++) {
	    for (y = 0; y < rows; y++) {
		Rast_row_update_fp_range(tile, cols, range, cellType);
		tile = G_incr_void_ptr(tile, map->tileX * Rast3d_length(type));
	    }
	    if (yRedundant)
		tile =
		    G_incr_void_ptr(tile,
				    map->tileX * yRedundant *
				    Rast3d_length(type));
	}
	return;
    }

    if (yRedundant) {
	for (z = 0; z < depths; z++) {
	    Rast_row_update_fp_range(tile, map->tileX * rows, range, cellType);
	    tile = G_incr_void_ptr(tile, map->tileXY * Rast3d_length(type));
	}
	return;
    }

    Rast_row_update_fp_range(tile, map->tileXY * depths, range, cellType);
}

/*---------------------------------------------------------------------------*/

int
Rast3d_readRange(const char *name, const char *mapset, struct FPRange *drange)
 /* adapted from Rast_read_fp_range */
{
    int fd;
    char xdr_buf[100];
    DCELL dcell1, dcell2;
    XDR xdr_str;

    Rast_init_fp_range(drange);

    fd = -1;

    fd = G_open_old_misc(RASTER3D_DIRECTORY, RASTER3D_RANGE_ELEMENT, name, mapset);
    if (fd < 0) {
	G_warning(_("Unable to open range file for [%s in %s]"), name, mapset);
	return -1;
    }

    if (read(fd, xdr_buf, 2 * RASTER3D_XDR_DOUBLE_LENGTH) != 2 * RASTER3D_XDR_DOUBLE_LENGTH) {
	close(fd);
	G_warning(_("Error reading range file for [%s in %s]"), name, mapset);
	return 2;
    }

    xdrmem_create(&xdr_str, xdr_buf, (u_int) RASTER3D_XDR_DOUBLE_LENGTH * 2,
		  XDR_DECODE);

    /* if the f_range file exists, but empty */
    if (!xdr_double(&xdr_str, &dcell1) || !xdr_double(&xdr_str, &dcell2)) {
	close(fd);
	G_warning(_("Error reading range file for [%s in %s]"), name, mapset);
	return -1;
    }

    Rast_update_fp_range(dcell1, drange);
    Rast_update_fp_range(dcell2, drange);
    close(fd);
    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Loads the range into the range structure of <em>map</em>.
 *
 *  \param map
 *  \return 1 ... if successful
 *          0 ... otherwise.
 */

int Rast3d_range_load(RASTER3D_Map * map)
{
    if (map->operation == RASTER3D_WRITE_DATA)
	return 1;
    if (Rast3d_readRange(map->fileName, map->mapset, &(map->range)) == -1) {
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Returns in <em>min</em> and <em>max</em> the minimum and maximum values of
 * the range.
 *
 *  \param map
 *  \param min
 *  \param max
 *  \return void
 */

void Rast3d_range_min_max(RASTER3D_Map * map, double *min, double *max)
{
    Rast_get_fp_range_min_max(&(map->range), min, max);
}

/*-------------------------------------------------------------------------*/

static int writeRange(const char *name, struct FPRange *range)
 /* adapted from Rast_write_fp_range */
{
    char xdr_buf[100];
    int fd;
    XDR xdr_str;

    fd = G_open_new_misc(RASTER3D_DIRECTORY, RASTER3D_RANGE_ELEMENT, name);
    if (fd < 0) {
	G_warning(_("Unable to open range file for <%s>"), name);
	return -1;
    }

    if (range->first_time) {
	/* if range hasn't been updated, write empty file meaning NULLs */
	close(fd);
	return 0;
    }

    xdrmem_create(&xdr_str, xdr_buf, (u_int) RASTER3D_XDR_DOUBLE_LENGTH * 2,
		  XDR_ENCODE);

    if (!xdr_double(&xdr_str, &(range->min)))
	goto error;
    if (!xdr_double(&xdr_str, &(range->max)))
	goto error;

    if (write(fd, xdr_buf, RASTER3D_XDR_DOUBLE_LENGTH * 2) != RASTER3D_XDR_DOUBLE_LENGTH * 2)
	goto error;

    close(fd);
    return 0;

  error:
    close(fd);
    G_remove_misc(RASTER3D_DIRECTORY, RASTER3D_RANGE_ELEMENT, name);	/* remove the old file with this name */
    G_warning("can't write range file for [%s in %s]", name, G_mapset());
    return -1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Writes the range which is stored in the range structure of <em>map</em>. 
 * (This function is invoked automatically when a new file is closed).
 *
 *  \param map
 *  \return 1 ... if successful
 *          0 ... otherwise.
 */

int Rast3d_range_write(RASTER3D_Map * map)
{
    char path[GPATH_MAX];

    Rast3d_filename(path, RASTER3D_RANGE_ELEMENT, map->fileName, map->mapset);
    remove(path);

    if (writeRange(map->fileName, &(map->range)) == -1) {
	Rast3d_error("Rast3d_closeCellNew: error in writeRange");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_range_init(RASTER3D_Map * map)
{
    Rast_init_fp_range(&(map->range));
    return 0;
}
