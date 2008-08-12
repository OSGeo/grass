/*!
  \file gis/get_cellhd.c
  
  \brief GIS library - Read raster map header
  
  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
   \author Original author CERL
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
  \brief Read the raster header
  
  The raster header for the raster map <i>name</i> in the specified
  <i>mapset</i> is read into the <i>cellhd</i> structure.  If there is
  an error reading the raster header file, a diagnostic message is
  printed and -1 is returned. Otherwise, 0 is returned.
  
  <b>Note</b>:a warning message for errors encountered.
  
  Cell header files may contain either grid cell header 
  information or reclass information.   If it is a reclass
  file, it will specify the map and mapset names of the actual
  grid cell file being reclassed.  G_get_cellhd(), upon 
  reading reclass information will go read the cell header
  information for the referenced file.  Only one reference is 
  allowed.
  
  \param name name of map
  \param mapset mapset that map belongs to
  \param cellhd structure to hold cell header info
  
  \return 0 on success
  \return -1 on error
 */

int G_get_cellhd(const char *name, const char *mapset,
		 struct Cell_head *cellhd)
{
    FILE *fd;
    int is_reclass;
    char real_name[GNAME_MAX], real_mapset[GMAPSET_MAX];
    char buf[1024];
    char *tail;
    char *err;

    /*
       is_reclass = G_is_reclass (name, mapset, real_name, real_mapset);
       if (is_reclass < 0)
       {
       sprintf (buf,"Can't read header file for [%s in %s]\n", name, mapset);
       tail = buf + strlen(buf);
       strcpy (tail, "It is a reclass file, but with an invalid format");
       G_warning(buf);
       return -1;
       }
     */
    is_reclass = (G_is_reclass(name, mapset, real_name, real_mapset) > 0);
    if (is_reclass) {
	fd = G_fopen_old("cellhd", real_name, real_mapset);
	if (fd == NULL) {
	    sprintf(buf,
		    _("Unable to read header file for raster map <%s@%s>."),
		    name, mapset);
	    tail = buf + strlen(buf);
	    sprintf(tail, _(" It is a reclass of raster map <%s@%s> "),
		    real_name, real_mapset);
	    tail = buf + strlen(buf);
	    if (!G_find_cell(real_name, real_mapset))
		sprintf(tail, _("which is missing."));
	    else
		sprintf(tail, _("whose header file can't be opened."));
	    G_warning(buf);
	    return -1;
	}
    }
    else {
	fd = G_fopen_old("cellhd", name, mapset);
	if (fd == NULL) {
	    G_warning(_("Unable to open header file for raster map <%s@%s>"),
		      name, mapset);
	    return -1;
	}
    }

    err = G__read_Cell_head(fd, cellhd, 1);
    fclose(fd);

    if (err == NULL)
	return 0;

    sprintf(buf, _("Unable to read header file for raster map <%s@%s>."),
	    name, mapset);
    tail = buf + strlen(buf);
    if (is_reclass) {
	sprintf(tail,
		_(" It is a reclass of raster map <%s@%s> whose header file is invalid."),
		real_name, real_mapset);
    }
    else
	sprintf(tail, _(" Invalid format."));
    tail = buf + strlen(buf);
    strcpy(tail, err);

    G_free(err);

    G_warning(buf);
    return -1;
}
