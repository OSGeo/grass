/*!
 * \file lib/raster/put_cellhd.c
 *
 * \brief Raster Library - Write raster header.
 *
 * SPDX-FileCopyrightText:  2001-2009 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
   \brief Writes the raster file header.

   Writes the cell file header information associated with map layer "map"
   into current mapset from the structure "cellhd".

   \param name name of map
   \param cellhd structure holding cell header info

   \return void
 */
void Rast_put_cellhd(const char *name, struct Cell_head *cellhd)
{
    FILE *fp;

    fp = G_fopen_new("cellhd", name);
    if (!fp)
        G_fatal_error(_("Unable to create header file for <%s>"), name);

    G__write_Cell_head(fp, cellhd, 1);
    fclose(fp);
}
