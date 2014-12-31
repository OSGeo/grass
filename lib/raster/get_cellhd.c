/*!
   \file lib/raster/get_cellhd.c

   \brief Raster library - Read raster map header

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/*!
   \brief Read the raster header

   The raster header for the raster map <i>name</i> in the specified
   <i>mapset</i> is read into the <i>cellhd</i> structure. If there is
   an error reading the raster header file, a diagnostic message is
   printed and -1 is returned. Otherwise, 0 is returned.

   <b>Note</b>:a warning message for errors encountered.

   Cell header files may contain either grid cell header information or
   reclass information. If it is a reclass file, it will specify the
   map and mapset names of the actual grid cell file being
   reclassed. Rast_get_cellhd(), upon reading reclass information will go
   read the cell header information for the referenced file. Only one
   reference is allowed.

   \param name name of map
   \param mapset mapset that map belongs to
   \param[out] cellhd structure to hold cell header info

   \return void
 */
void Rast_get_cellhd(const char *name, const char *mapset,
		     struct Cell_head *cellhd)
{
    FILE *fp;
    int is_reclass;
    char real_name[GNAME_MAX], real_mapset[GMAPSET_MAX];

    /*
       is_reclass = Rast_is_reclass (name, mapset, real_name, real_mapset);
       if (is_reclass < 0)
       {
       sprintf (buf,"Can't read header file for [%s in %s]\n", name, mapset);
       tail = buf + strlen(buf);
       strcpy (tail, "It is a reclass file, but with an invalid format");
       G_warning(buf);
       return -1;
       }
     */
    is_reclass = (Rast_is_reclass(name, mapset, real_name, real_mapset) > 0);
    if (is_reclass) {
	fp = G_fopen_old("cellhd", real_name, real_mapset);
	if (!fp)
	    G_fatal_error(_("Unable to read header file for raster map <%s@%s>. "
			    "It is a reclass of raster map <%s@%s> %s"),
			  name, mapset, real_name, real_mapset,
			  !G_find_raster(real_name, real_mapset)
			  ? _("which is missing.")
			  : _("whose header file can't be opened."));
    }
    else {
	fp = G_fopen_old("cellhd", name, mapset);
	if (!fp)
	    G_fatal_error(_("Unable to open header file for raster map <%s@%s>"),
			  name, mapset);
    }

    G__read_Cell_head(fp, cellhd, 1);
    fclose(fp);
}
