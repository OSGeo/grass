/*!
  \file lib/db/dbmi_base/isdir.c
  
  \brief DBMI Library (base) - test for directories
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/config.h>
#include <grass/dbmi.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*!
  \brief Test if path is a directory

  \param path pathname

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_isdir(const char *path)
{
    struct stat x;

    if (stat(path, &x) != 0)
	return DB_FAILED;
    return (S_ISDIR(x.st_mode) ? DB_OK : DB_FAILED);
}
