#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <grass/gis.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

void
G3d_range_updateFromTile(G3D_Map * map, const void *tile, int rows, int cols,
			 int depths, int xRedundant, int yRedundant,
			 int zRedundant, int nofNum, int type)
{
    int y, z, cellType;
    struct FPRange *range;

    range = &(map->range);
    cellType = G3d_g3dType2cellType(type);

    if (nofNum == map->tileSize) {
	G_row_update_fp_range(tile, map->tileSize, range, cellType);
	return;
    }

    if (xRedundant) {
	for (z = 0; z < depths; z++) {
	    for (y = 0; y < rows; y++) {
		G_row_update_fp_range(tile, cols, range, cellType);
		tile = G_incr_void_ptr(tile, map->tileX * G3d_length(type));
	    }
	    if (yRedundant)
		tile =
		    G_incr_void_ptr(tile,
				    map->tileX * yRedundant *
				    G3d_length(type));
	}
	return;
    }

    if (yRedundant) {
	for (z = 0; z < depths; z++) {
	    G_row_update_fp_range(tile, map->tileX * rows, range, cellType);
	    tile = G_incr_void_ptr(tile, map->tileXY * G3d_length(type));
	}
	return;
    }

    G_row_update_fp_range(tile, map->tileXY * depths, range, cellType);
}

/*---------------------------------------------------------------------------*/

int
G3d_readRange(const char *name, const char *mapset, struct FPRange *drange)
 /* adapted from G_read_fp_range */
{
    int fd;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char buf[GNAME_MAX + sizeof(G3D_DIRECTORY) + 2],
	buf2[GMAPSET_MAX + sizeof(G3D_RANGE_ELEMENT) + 2];
    char xdr_buf[100];
    DCELL dcell1, dcell2;
    XDR xdr_str;

    G_init_fp_range(drange);

    fd = -1;

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, xname);
	sprintf(buf2, "%s@%s", G3D_RANGE_ELEMENT, xmapset);	/* == range@mapset */
    }
    else {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, name);
	sprintf(buf2, "%s", G3D_RANGE_ELEMENT);
    }

    if (G_find_file2(buf, buf2, mapset)) {
	fd = G_open_old(buf, buf2, mapset);
	if (fd < 0)
	    goto error;

	if (read(fd, xdr_buf, 2 * G3D_XDR_DOUBLE_LENGTH) !=
	    2 * G3D_XDR_DOUBLE_LENGTH)
	    return 2;

	xdrmem_create(&xdr_str, xdr_buf, (u_int) G3D_XDR_DOUBLE_LENGTH * 2,
		      XDR_DECODE);

	/* if the f_range file exists, but empty */
	if (!xdr_double(&xdr_str, &dcell1) || !xdr_double(&xdr_str, &dcell2))
	    goto error;

	G_update_fp_range(dcell1, drange);
	G_update_fp_range(dcell2, drange);
	close(fd);
	return 1;
    }

  error:
    if (fd > 0)
	close(fd);
    G_warning("can't read range file for [%s in %s]", name, mapset);
    return -1;
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

int G3d_range_load(G3D_Map * map)
{
    if (map->operation == G3D_WRITE_DATA)
	return 1;
    if (G3d_readRange(map->fileName, map->mapset, &(map->range)) == -1) {
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

void G3d_range_min_max(G3D_Map * map, double *min, double *max)
{
    G_get_fp_range_min_max(&(map->range), min, max);
}

/*-------------------------------------------------------------------------*/

static int writeRange(const char *name, struct FPRange *range)
 /* adapted from G_write_fp_range */
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char buf[GNAME_MAX + sizeof(G3D_DIRECTORY) + 2],
	buf2[GMAPSET_MAX + sizeof(G3D_RANGE_ELEMENT) + 2];
    char xdr_buf[100];
    int fd;
    XDR xdr_str;

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, xname);
	sprintf(buf2, "%s@%s", G3D_RANGE_ELEMENT, xmapset);	/* == range@mapset */
    }
    else {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, name);
	sprintf(buf2, "%s", G3D_RANGE_ELEMENT);
    }

    fd = G_open_new(buf, buf2);
    if (fd < 0)
	goto error;

    if (range->first_time) {
	/* if range hasn't been updated, write empty file meaning NULLs */
	close(fd);
	return 0;
    }

    xdrmem_create(&xdr_str, xdr_buf, (u_int) G3D_XDR_DOUBLE_LENGTH * 2,
		  XDR_ENCODE);

    if (!xdr_double(&xdr_str, &(range->min)))
	goto error;
    if (!xdr_double(&xdr_str, &(range->max)))
	goto error;

    write(fd, xdr_buf, G3D_XDR_DOUBLE_LENGTH * 2);
    close(fd);
    return 0;

  error:
    G_remove(buf, buf2);	/* remove the old file with this name */
    sprintf(buf, "can't write range file for [%s in %s]", name, G_mapset());
    G_warning(buf);
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

int G3d_range_write(G3D_Map * map)
{
    char path[GPATH_MAX];

    G3d_filename(path, G3D_RANGE_ELEMENT, map->fileName, map->mapset);
    remove(path);

    if (writeRange(map->fileName, &(map->range)) == -1) {
	G3d_error("G3d_closeCellNew: error in writeRange");
	return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_range_init(G3D_Map * map)
{
    return G_init_fp_range(&(map->range));
}
