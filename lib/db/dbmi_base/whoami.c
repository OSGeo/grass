/*!
  \file lib/db/dbmi_base/whoami.c
  
  \brief DBMI Library (base) - who am i
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>

/*!
  \brief Who am i?

  Check environmental variable LOGNAME

  \return string buffer with logname
*/
const char *db_whoami()
{
    return G_store(getenv("LOGNAME"));
}
