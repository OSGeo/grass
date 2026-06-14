/*!
 * \file lib/gis/myname.c
 *
 * \brief GIS Library - Database name functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
*
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Returns location title.
 *
 * Returns a one line title for the database location. This title is
 * read from the file MYNAME in the PERMANENT mapset. See also \ref
 * Permanent_Mapset for a discussion of the PERMANENT mapset.
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
        strcpy(name, _("This location has no description."));

    return G_store(name);
}
