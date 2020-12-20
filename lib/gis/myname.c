/*!
 * \file lib/gis/myname.c
 *
 * \brief GIS Library - Database name functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Returns project title.
 *
 * Returns a one line title for the database project. This title is
 * read from the file MYNAME in the PERMANENT subproject. See also \ref
 * Permanent_Subproject for a discussion of the PERMANENT subproject.
 *
 * <b>Note:</b> This name is the first line in the file 
 * $GISDBASE/$LOCATION_NAME/PERMANENT/MYNAME
 *
 * \return pointer to a string
 */

char *G_myname(void)
{
    char name[GNAME_MAX];
    char path[GPATH_MAX];
    FILE *fd;
    int ok;

    ok = 0;

    G_file_name(path, "", "MYNAME", "PERMANENT");
    if ((fd = fopen(path, "r"))) {
	ok = G_getl(name, sizeof name, fd);
	fclose(fd);
    }
    if (!ok)
	strcpy(name, _("This project has no description."));

    return G_store(name);
}
