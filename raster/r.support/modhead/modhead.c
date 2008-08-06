#include <grass/config.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/edit.h>
#include "local_proto.h"


/*
 *******************************************************************
 *
 * MODULE:        modhead (etc/support) for r.support
 *
 * AUTHOR(S):     Original by Michael Shapiro - CERL
 *                Port to 6.x by Brad Douglas
 *
 * PURPOSE:       r.support module that allows editing of raster
 *                file header.
 *
 * COPYRIGHT:     (C) 2000-2005 by the GRASS Development Team
 *
 *                This program is free software under the GNU General
 *                Public License (>=v2). Read the file COPYING that
 *                comes with GRASS for details.
 *
 ********************************************************************/

int main(int argc, char *argv[])
{
    char buffer[256];
    struct Cell_head cellhd;
    int cellhd_ok;
    int compressed_old;
    int compressed_new;
    int fd;
    off_t filesize;
    off_t offset;
    off_t prev_offset = 0;
    int rows_old = 0;
    int rows_new = 0;
    int quiet = 0;
    char *rname = NULL;
    char *rmapset = NULL;
    char *mapset;
    char name[64];


    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    if (argc == 3) {
	if (strcmp(argv[1], "-") == 0) {
	    argv++;
	    argc--;
	    quiet = 1;
	}
    }

    if (argc >= 2) {
	strncpy(name, argv[1], sizeof(name));
	G_message(_("Edit header for [%s]\n"), name);

	if ((mapset = G_find_cell(name, "")) == NULL)
	    G_fatal_error(_("[%s] cannot be found!"), name);
    }
    else {
	mapset = G_ask_cell_in_mapset(_("For what layer shall the "
					"header file be edited? "), name);

	if (mapset == NULL)
	    return EXIT_SUCCESS;
    }

    /* Make sure map is not reclassed */
    if (G_is_reclass(name, mapset, rname, rmapset) > 0) {
	if (strcmp(mapset, rmapset) == 0)
	    G_fatal_error(_("[%s] is a reclass of [%s] - cannot edit header! Run support on [%s]."),
			  name, rname, rname);

	/* ELSE */
	G_fatal_error(_("[%s] is a reclass of [%s in %s] - cannot edit header!"),
		      name, rname, rmapset);
    }

    /* Open raster map */
    if ((fd = G_open_cell_old(name, mapset)) < 0)
	G_fatal_error(_("Cannot open raster map [%s]!"), name);

    /* Determine file size */
    filesize = lseek(fd, 0L, SEEK_CUR);
    if (filesize == 0)
	G_fatal_error(_("Raster file [%s] is empty."), name);
    else if (filesize < 0)
	G_fatal_error(_("Error reading raster map [%s]."), name);

    G_suppress_warnings(quiet);
    cellhd_ok = (G_get_cellhd(name, mapset, &cellhd) >= 0);
    G_suppress_warnings(0);
    if (!cellhd_ok) {
	G_zero(&cellhd, (int)sizeof(cellhd));
	cellhd.proj = G_projection();
	cellhd.zone = G_zone();
    }
    else
	cellhd.format++;	/* set to number of bytes per cell (for now) */

    /*
     * Determine compression type, if any, without consulting cellhd
     *
     * In a compressed file, there is an array of row addresses at the
     * beginning of the file.  Try to read the address array.
     * If the file really is compressed, the addresses will increase,
     * the last one will be the same as the filesize, and the number of
     * row addresses will be one more than the number of rows in the file.
     *
     * If the file matches these conditions, it is probably compressed.
     * The probability of this being wrong is very small.
     * So we will take a safe route that doesn't annoy the user:
     *  If the cellhd wasn't valid, verify the compression with the user.
     *  else if the cellhd says something different, ask the user.
     *  else don't bother the user about it.
     *
     * note: 3.0 address are in machine independent format
     *       pre 3.0 are true longs
     */

    /* Look for pre3.0 compression */
    compressed_old = 0;
    lseek(fd, 0L, SEEK_SET);
    if (read(fd, buffer, (size_t) 3) == 3 && buffer[0] == (char)251 &&
	buffer[1] == (char)255 && buffer[2] == (char)251) {

	rows_old = 0;
	offset = -1;

	while ((next_row_addr(fd, &offset, 0)) == EXIT_SUCCESS) {
	    if (rows_old > 0 && offset <= prev_offset)
		break;
	    if (offset >= filesize)
		break;
	    prev_offset = offset;
	    rows_old++;
	}

	if (offset == filesize)
	    /* It is old format compressed */
	    compressed_old = 1;
    }

    /* Look for 3.0 compression */
    compressed_new = 0;
    lseek(fd, 0L, SEEK_SET);
    if (read(fd, buffer, (size_t) 1) == 1 && buffer[0] > 0) {
	int nbytes;

	nbytes = buffer[0];
	rows_new = 0;
	offset = -1;

	while ((next_row_addr(fd, &offset, nbytes)) == EXIT_SUCCESS) {
	    if (rows_new > 0 && offset <= prev_offset)
		break;
	    if (offset >= filesize)
		break;

	    prev_offset = offset;
	    rows_new++;
	}
    }

    if (offset == filesize)
	compressed_new = 1;

    /*
     * now check these results against cellhd.compressed
     * cellhd.compressed values are
     * -1 pre 3.0 cellhd - compression unknown (by cellhd alone)
     *  0 not compressed (3.0)
     *  1 compressed (3.0)
     */

    G_message(_("cellhd compression: %d\n"), cellhd.compressed);
    G_message(_("3.0 compression %sindicated\n"),
	      compressed_new ? "" : "not ");
    G_message(_("Pre 3.0 compression %sindicated\n"),
	      compressed_old ? "" : "not ");
    hitreturn();

    /* If we create a new cell header, find out if file is compressed */
    if (!cellhd_ok) {
	G_snprintf(buffer, sizeof(buffer),
		   _("[%s] appears to be compressed. Is it? "), name);
	cellhd.compressed = 0;

	if ((compressed_new || compressed_old) && G_yes(buffer, -1)) {
	    if (compressed_new && compressed_old) {
		while (1) {
		    G_message(_("Please indicate the type of compression:\n"));
		    G_message(_("  1. Pre 3.0 compression\n"));
		    G_message(_("  2. 3.0 compression\n"));
		    if (!G_gets(buffer))
			continue;

		    G_strip(buffer);
		    if (strcmp(buffer, "1") == 0)
			break;
		    if (strcmp(buffer, "2") == 0)
			break;
		}

		switch (*buffer) {
		case '1':
		    compressed_new = 0;
		    break;
		case '2':
		    compressed_old = 0;
		    break;
		}
	    }

	    if (compressed_new) {
		cellhd.compressed = 1;
		cellhd.rows = rows_new;
	    }
	    else {
		cellhd.compressed = -1;
		cellhd.rows = rows_old;
	    }
	}
    }
    else {
	if ((cellhd.compressed < 0) && !compressed_old)
	    cellhd.compressed = 0;

	if ((cellhd.compressed == 0) && compressed_new) {
	    G_warning(_("The header for [%s] says the file is not compressed. "),
		      name);
	    G_warning(_("The file appears to be compressed.\n"));
	    G_warning(_("Most likely the header is wrong, but I want you to decide.\n"));

	    if (G_yes("Is the file compressed? ", -1))
		cellhd.compressed = 1;
	}
	else if ((cellhd.compressed != 0) && !compressed_new) {
	    G_warning(_("The header for [%s] says the file is compressed. "),
		      name);
	    G_warning(_("The file does NOT appear to be compressed.\n"));
	    G_warning(_("Most likely the header is wrong, but I want you to decide.\n"));

	    if (!G_yes("Is the file really compressed? ", -1))
		cellhd.compressed = 0;
	}
    }

    if ((cellhd.compressed < 0 && rows_old != cellhd.rows) ||
	(cellhd.compressed > 0 && rows_new != cellhd.rows)) {

	int rows;

	rows = (cellhd.compressed > 0 ? rows_new : rows_old);

	G_warning(_("Header indicates %d row%s in the raster map, but "
		    "the actual file format indicates %d row%s"),
		  cellhd.rows, (cellhd.rows == 1) ? "" : "s",
		  rows, (rows == 1) ? "" : "s");

	if (G_yes("Should this discrepancy be corrected? ", -1))
	    cellhd.rows = rows;
    }

    while (1) {
	ask_format(name, &cellhd, filesize);

	if (cellhd.compressed == 0) {
	    if (check_uncompressed(&cellhd, filesize) == EXIT_SUCCESS)
		break;
	}
	else
	    break;

	hitreturn();
    }

    if (E_edit_cellhd(&cellhd, 1) < 0)
	return EXIT_SUCCESS;

    /* adjust from nbytes to nbytes-1 */
    /* FP map should be back to -1 */
    /* if (cellhd.format > 0) */
    cellhd.format--;

    /* Write new header out */
    if (G_put_cellhd(name, &cellhd) == -1)
	G_fatal_error(_("Unable to write header for [%s]."), name);
    else
	G_message(_("Header for raster map [%s] updated."), name);

    /* Free resources */
    close(fd);

    return EXIT_SUCCESS;
}
