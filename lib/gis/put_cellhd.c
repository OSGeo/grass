/*!
 * \file gis/put_cellhd.c
 *
 * \brief GIS Library - Write raster header.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
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

  \return 0 on success
  \return -1 on failure
*/
int G_put_cellhd(const char *name, struct Cell_head *cellhd)
{
    FILE *fd;

    if (!(fd = G_fopen_new("cellhd", name))) {
	G_warning(_("Unable to create header file for <%s>"), name);
	return -1;
    }

    G__write_Cell_head(fd, cellhd, 1);
    fclose(fd);

    return 0;
}
