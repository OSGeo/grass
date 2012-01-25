/*!
  \file db/driver/postgres/error.c
  
  \brief DBMI - Low Level PostgreSQL database driver - report errors
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
*/

#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

#include "globals.h"


/* init error message */
void init_error(void)
{
    db_d_init_error("PostgreSQL");
}
