/*!
   \file db/drivers/driver.c

   \brief Low level OGR SQL driver

   (C) 2004-2009 by the GRASS Development Team
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Some updates by Martin Landa <landa.martin gmail.com>
 */

#include <grass/dbmi.h>

#include "ogr_api.h"
#include "globals.h"
#include "proto.h"

/*!
   \brief Initialize driver

   \param argc number of arguments
   \param argv array of arguments

   \return DB_OK on success
 */
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_init(int argc UNUSED, char *argv[] UNUSED)
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
int db__driver_init(int argc, char *argv[])
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    init_error();
    return DB_OK;
}

/*!
   \brief Finish driver

   \return DB_OK
 */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_finish(void)
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
int db__driver_finish()
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_finish(void)
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
int db__driver_finish()
=======
int db__driver_finish(void)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
{
    return DB_OK;
}
