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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
const char *db_whoami(void)
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
const char *db_whoami()
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
const char *db_whoami(void)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
{
    return G_store(getenv("LOGNAME"));
}
