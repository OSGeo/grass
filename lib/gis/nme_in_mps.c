/*!
   \file nme_in_mps.c

   \brief GIS Library - check map name

   (C) 2001-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>

/*!
   \brief Checks to see if 'name_in' is in the format: <name> in <mapset>

   \param name_in full map name
   \param[out] name_out map name
   \param[out] mapset mapset name

   \return 1 name_in is in this format.
   name_out will contain the simple <name>
   mapset will contain <mapset>
   \return 0 name_in is not in this format
   name_out and mapset are undefined (changed)
 */
#ifndef COMMENTED_OUT
int G__name_in_mapset(const char *name_in, char *name_out, char *mapset)
{
    char in[1024];

    *in = 0;
    return (sscanf(name_in, "%s %s %s", name_out, in, mapset) == 3 &&
	    strcmp(in, "in") == 0);
}
#endif

/*!
   \brief Check if map name is fully qualified (map @ mapset)

   Note:
   - <b>name</b> is char array of size GNAME_MAX
   - <b>mapset</b> is char array of size GMAPSET_MAX

   \param fullname full map name
   \param[out] name map name
   \param[out] mapset mapset name

   \return 1 if input map name is fully qualified
   \return 0 if input map name is not fully qualified
 */
int G__name_is_fully_qualified(const char *fullname, char *name, char *mapset)
{
    const char *p;
    char *q;

    /* search for name@mapset */

    *name = *mapset = 0;

    for (p = fullname; *p; p++)
	if (*p == '@')
	    break;

    if (*p == 0)
	return 0;

    /* copy the name part */
    q = name;
    while (fullname != p)
	*q++ = *fullname++;
    *q = 0;

    /* copy the mapset part */
    p++;			/* skip the @ */
    q = mapset;
    while ((*q++ = *p++)) ;

    return (*name && *mapset);
}


/*!
   \brief fully qualified file name

   Returns a fully qualified name for the file <b>name</b> in 
   <b>mapset.</b> Currently this string is in the form <i>name @ mapset</i>, 
   but the programmer should pretend not to know this and always call this 
   routine to get the fully qualified name.
   The following example shows how an interactive version of <i>d.rast</i>
   interfaces with the command-line version of <i>d.rast</i>:

   \code
   #include "gis.h"
   int main(char *argc, char **argv)
   {
   char name[GNAME_MAX], *mapset, *fqn;
   char command[1024];
   G_gisinit(argv[0]);
   mapset = G_ask_cell_old ("", name, "");
   if (mapset == NULL) exit(EXIT_SUCCESS);
   fqn = G_fully_qualified_name (name, mapset);
   sprintf (command, "d.rast map='%s'", fqn);
   system(command);
   }
   \endcode

   \param name map name
   \param mapset mapset name

   \return pointer to full map name (map @ mapset)
 */
char *G_fully_qualified_name(const char *name, const char *mapset)
{
    char fullname[GNAME_MAX + GMAPSET_MAX];

    if (strchr(name, '@'))
	sprintf(fullname, "%s", name);
    else
	sprintf(fullname, "%s@%s", name, mapset);

    return G_store(fullname);
}
