/*!
  \file lib/gis/nme_in_mps.c

  \brief GIS Library - check map name

  (C) 2001-2009, 2013 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <string.h>
#include <grass/gis.h>

/*!
  \brief Check if map name is fully qualified (map @ mapset)
  
  Returns a fully qualified name for the file <i>name</i> in
  <i>mapset</i>. Currently this string is in the form
  <i>name@mapset</i>, but the programmer should pretend not to know this
  and always call this routine to get the fully qualified name.

  Note:
   - <i>name</i> is char array of size GNAME_MAX
   - <i>mapset</i> is char array of size GMAPSET_MAX

  \param fullname full map name
  \param[out] name map name
  \param[out] mapset mapset name
  
  \return 1 if input map name is fully qualified
  \return 0 if input map name is not fully qualified
 */
int G_name_is_fully_qualified(const char *fullname, char *name, char *mapset)
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
   \brief Get fully qualified element name

   Returns a fully qualified name for GIS element <i>name</i> in
   <i>mapset</i>. Currently this string is in the form
   <b>name@mapset</b>, but the programmer should pretend not to know
   this and always call this routine to get the fully qualified name.

   String is allocated by G_store().
   
   \code
   #include <grass/gis.h>
   int main(char *argc, char **argv)
   {
       char name[GNAME_MAX], *mapset, *fqn;
       char command[1024];

       G_gisinit(argv[0]);
       mapset = G_find_rast(name, "");
       if (mapset == NULL)
           exit(EXIT_SUCCESS);

       fqn = G_fully_qualified_name (name, mapset);
       printf (stdout, "map='%s'", fqn);

       exit(EXIT_SUCCESS);
   }
   \endcode

   \param name element name
   \param mapset mapset name

   \return pointer to full element name (map@mapset)
 */
char *G_fully_qualified_name(const char *name, const char *mapset)
{
    char fullname[GNAME_MAX + GMAPSET_MAX];

    if (strchr(name, '@') || strlen(mapset) < 1) {
	sprintf(fullname, "%s", name);
    }
    else {
	sprintf(fullname, "%s@%s", name, mapset);
    }

    return G_store(fullname);
}

/*!
  \brief Returns unqualified map name (without @ mapset)

  Returns an unqualified name for the file <i>name</i> in
  <i>mapset</i>.

  Note:
   - <i>name, xname</i> are char array of size GNAME_MAX
   - <i>mapset, xmapset</i> are char array of size GMAPSET_MAX

  \param fullname map name
  \param map mapset to check or NULL
  \param[out] name map name
  \param[out] mapset mapset name

  \return  1 if input map name is fully qualified
  \return  0 if name is not fully qualified
  \return -1 if input mapset invalid (mapset != xmapset)
 */
int G_unqualified_name(const char *name, const char *mapset,
		       char *xname, char *xmapset)
{
    if (G_name_is_fully_qualified(name, xname, xmapset)) {
        /* name is fully qualified */
	if (mapset && *mapset && strcmp(mapset, xmapset) != 0)
	    return -1;
	return 1;
    }

    /* name is not fully qualified */
    strcpy(xname, name);
    if (mapset)
        strcpy(xmapset, mapset);
    else
        xmapset[0] = '\0';
    
    return 0;
}
