/*!
 * \file lib/gis/color_print.c
 *
 * \brief GIS Library - Print color table of raster map
 *
 * (C) 2010 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Martin Landa <landa.martin gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include <grass/gis.h>

static int print_color_table(const char *, const char *, const char *, FILE *);

/*!
  \brief Print current color table of raster map to file

  \param name name of raster map
  \param mapset mapset of raster map
  \param file file where to print color table

  \return -1 on error (raster map not found)
  \return -2 on error (reading color table failed)
  \return 0 on success
*/
int Rast_print_colors(const char *name, const char *mapset, FILE *file)
{
    char element[512];
    char xname[GNAME_MAX];
        
    strcpy(xname, name);
    mapset = G_find_raster(xname, mapset);
    if (!mapset)
	return -1;
    name = xname;
    
    /* first look for secondary color table in current mapset */
    sprintf(element, "colr2/%s", mapset);
    if (print_color_table(element, name, G_mapset(), file) < 0)
	/* now look for the regular color table */
	if (print_color_table("colr", name, mapset, file) < 0)
	    return -2;
    
    return 0;
}

int print_color_table(const char *element, const char *name,
		      const char *mapset, FILE *file)
{
    FILE *fd;
    char buf[4096];
    
    fd = G_fopen_old(element, name, mapset);
    if (!fd)
	return -1;
    
    while(fgets(buf, sizeof(buf), fd)) {
	fwrite(buf, strlen(buf), 1, file);
    }
    
    fclose(fd);
    
    return 0;
}
