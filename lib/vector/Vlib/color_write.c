/*!
  \file lib/vector/Vlib/color_write.c
 
  \brief Vector Library - write color table for vector map
  
  (C) 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/*!
  \brief Write color table for vector map
  
  The color table is written for the vector map <i>name</i> in the
  specified <i>mapset</i> from the <i>colors</i> structure.
  
  The <i>colors</i> structure must be created properly, i.e.,
  Rast_init_colors() to initialize the structure and
  Rast_add_c_color_rule() to set the category colors. These routines
  are called by higher level routines which read or create entire
  color tables, such as Rast_read_colors() or Rast_make_ramp_colors().
  
  <b>Note:</b> The calling sequence for this function deserves
  special attention. The <i>mapset</i> parameter seems to imply that
  it is possible to overwrite the color table for a vector map which
  is in another mapset. However, this is not what actually
  happens. It is very useful for users to create their own color
  tables for vector maps in other mapsets, but without overwriting
  other users' color tables for the same raster map. If <i>mapset</i>
  is the current mapset, then the color file for <i>name</i> will be
  overwritten by the new color table. But if <i>mapset</i> is not the
  current mapset, then the color table is actually written in the
  current mapset under the <tt>colr2</tt> element as:
  <tt>vector/name/colr2</tt>.
  
  The rules are written out using floating-point format, removing
  trailing zeros (possibly producing integers). The flag marking the
  colors as floating-point is <b>not</b> written.
  
  If the environment variable FORCE_GRASS3_COLORS is set (to anything
  at all) then the output format is 3.0, even if the structure
  contains 4.0 rules.  This allows users to create 3.0 color files for
  export to sites which don't yet have 4.0
  
  \param name vector map name
  \param mapset mapset name
  \param colors pointer to structure Colors which holds color info
  
  \return void
*/
void Vect_write_colors(const char *name, const char *mapset,
		       struct Colors *colors)
{
    char element[GPATH_MAX];
    const char *cname;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    FILE *fd;
    
    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	if (strcmp(xmapset, mapset) != 0)
	    G_fatal_error(_("Qualified name <%s> doesn't match mapset <%s>"),
			  name, mapset);
	name = xname;
	mapset = xmapset;
    }
    
    /*
      if mapset is current mapset, write original color table
      else write secondary color table
    */
    if (strcmp(mapset, G_mapset()) == 0) {
	cname = GV_COLR_ELEMENT;
	sprintf(element, "%s/%s", GV_DIRECTORY, name);
    }
    else {
	cname = name;
	sprintf(element, "%s/%s", GV_COLR2_DIRECTORY, mapset);
    }

    if (!(fd = G_fopen_new(element, cname)))
	G_fatal_error(_("Unable to create <%s> file for map <%s>"),
		      element, name);

    Rast__write_colors(fd, colors);
    fclose(fd);
}
