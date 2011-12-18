/*!
   \file lib/imagery/iclass_bands.c

   \brief Imagery library - functions for wx.iclass

   Computation based on training areas for supervised classification.
   Based on i.class module (GRASS 6).

   Reading bands cell category values.

   Copyright (C) 1999-2007, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author David Satnik, Central Washington University (original author)
   \author Markus Neteler <neteler itc.it> (i.class module)
   \author Bernhard Reiter <bernhard intevation.de> (i.class module)
   \author Brad Douglas <rez touchofmadness.com>(i.class module)
   \author Glynn Clements <glynn gclements.plus.com> (i.class module)
   \author Hamish Bowman <hamish_b yahoo.com> (i.class module)
   \author Jan-Oliver Wagner <jan intevation.de> (i.class module)
   \author Anna Kratochvilova <kratochanna gmail.com> (rewriting for wx.iclass)
   \author Vaclav Petras <wenzeslaus gmail.com> (rewriting for wx.iclass)
 */

#include <grass/imagery.h>
#include <grass/raster.h>

#include "iclass_local_proto.h"

/*!
   \brief Open and allocate space for the group band files.

   \param refer pointer to band files structure
   \param[out] band_buffer buffer to read one row of each band
   \param[out] band_fd band files descriptors
 */
void open_band_files(struct Ref *refer, CELL *** band_buffer, int **band_fd)
{
    int n, nbands;

    char *name, *mapset;

    G_debug(3, "open_band_files()");

    /* allocate row buffers and open raster maps */
    nbands = refer->nfiles;
    *band_buffer = (CELL **) G_malloc(nbands * sizeof(CELL *));
    *band_fd = (int *)G_malloc(nbands * sizeof(int));

    for (n = 0; n < nbands; n++) {
	(*band_buffer)[n] = Rast_allocate_c_buf();
	name = refer->file[n].name;
	mapset = refer->file[n].mapset;
	(*band_fd)[n] = Rast_open_old(name, mapset);
    }
}

/*!
   \brief Close and free space for the group band files.

   \param refer pointer to band files structure
   \param band_buffer buffer to read one row of each band
   \param band_fd band files descriptors
 */
void close_band_files(struct Ref *refer, CELL ** band_buffer, int *band_fd)
{
    int n, nbands;

    G_debug(3, "close_band_files()");

    nbands = refer->nfiles;
    for (n = 0; n < nbands; n++) {
	G_free(band_buffer[n]);
	Rast_close(band_fd[n]);
    }

    G_free(band_buffer);
    G_free(band_fd);
}

/*!
   \brief Read one row of each band.

   \param band_buffer buffer to read one row of each band
   \param band_fd band files descriptors
   \param nbands number of band files
   \param row data row
 */
void read_band_row(CELL ** band_buffer, int *band_fd, int nbands, int row)
{
    int i;

    G_debug(5, "read_band_row(): row = %d", row);

    for (i = 0; i < nbands; i++)
	Rast_get_c_row_nomask(band_fd[i], band_buffer[i], row);
}
