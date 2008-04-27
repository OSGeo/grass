/**
 * \file myname.c
 *
 * \brief Database name functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/**
 * \fn char *G_myname ()
 *
 * \brief Returns location title.
 *
 * Returns a one line title for the database location. This title is 
 * read from the file MYNAME in the PERMANENT mapset. See also 
 * Permanent_Mapset for a discussion of the PERMANENT mapset.<br>
 *
 * <b>Note:</b> This name is the first line in the file 
 * $GISDBASE/$LOCATION_NAME/PERMANENT/MYNAME
 *
 * \return Pointer to a string
 */

char *
G_myname(void)
{
    static char name[GNAME_MAX];
    char path[GPATH_MAX];
    FILE *fd;
    int ok;

    ok = 0;

    G__file_name (path,"","MYNAME","PERMANENT");
    if ((fd = fopen(path,"r")))
    {
	ok = G_getl(name, sizeof name, fd);
	fclose (fd);
    }
    if (!ok)
	strcpy(name, _("Unknown Location")) ;

    return name;
}
