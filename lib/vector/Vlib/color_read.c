/*!
  \file lib/vector/Vlib/color_read.c
 
  \brief Vector Library - read color table of vector map
  
  (C) 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
  \brief Read color table of vector map
  
  The color table for the vector map <i>name</i> in the specified
  <i>mapset</i> is read into the <i>colors</i> structure. 
  
  Note: If a secondary color file for map name <i>name</i> exists in
  the current mapset, that color file is read. This allows the user to
  define their own color lookup tables for vector maps found in other
  mapsets.
  
  Warning message is printed if the color file is missing or invalid.
  
  \param name vector map name
  \param mapset mapset name ("" for search path)
  \param[out] colors pointer to Colors structure (can be NULL)
  
  \return -1 on error
  \return 0 if color table missing
  \return 1 on success (color table found)
*/
int Vect_read_colors(const char *name, const char *mapset,
		     struct Colors *colors)
{
    int ret;
    char buf[GPATH_MAX];
    char xname[GNAME_MAX];
    
    if (colors)
        Rast_init_colors(colors);
    
    strcpy(xname, name);
    mapset = G_find_vector(xname, mapset);
    if (!mapset)
	return -1;
    
    name = xname;

    if (strcmp(mapset, G_mapset()) == 0) {
        /* look for the regular color table */
	sprintf(buf, "%s/%s", GV_DIRECTORY, name);
        ret = Rast__read_colors(buf, GV_COLR_ELEMENT, mapset, colors);
    }
    else {
	/* look for secondary color table in current mapset */
	sprintf(buf, "%s/%s", GV_COLR2_DIRECTORY, mapset);
        ret = Rast__read_colors(buf, name, G_mapset(), colors);
    }
    if (ret == -2)
	return 0;

    return ret;
}
