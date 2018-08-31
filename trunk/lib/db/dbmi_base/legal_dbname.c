/*!
  \file lib/db/dbmi_base/legal_dbname.c
  
  \brief DBMI Library (base) - validate DB names

  \todo Are we as restrictive here as for vector names?
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
  \brief Check if output is legal table name
  
  Rule:  [A-Za-z][A-Za-z0-9_@]*
  \param s table name to be checked

  \return 1 OK 
  \return -1 if name does not start with letter A..Za..z or if name does
  not continue with A..Za..z0..9_@
*/
int db_legal_tablename(const char *s)
{
    char buf[GNAME_MAX];

    sprintf(buf, "%s", s);

    if (*s == '.' || *s == 0) {
	G_warning(_("Illegal table map name <%s>. May not contain '.' or 'NULL'."),
		  buf);
	return DB_FAILED;
    }

    /* file name must start with letter */
    if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z'))) {
	G_warning(_("Illegal table map name <%s>. Must start with a letter."),
		  buf);
	return DB_FAILED;
    }

    for (s++; *s; s++)
	if (!
	    ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') ||
	     (*s >= '0' && *s <= '9') || *s == '_' || *s == '@')) {
	    G_warning(_("Illegal table map name <%s>. Character <%c> not allowed."),
		      buf, *s);
	    return DB_FAILED;
	}

    return DB_OK;
}
