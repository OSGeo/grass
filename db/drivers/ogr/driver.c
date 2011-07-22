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
int db__driver_init(int argc, char *argv[])
{
    init_error();
    return DB_OK;
}

/*!
  \brief Finish driver

  \return DB_OK
*/
int db__driver_finish()
{
    return DB_OK;
}
