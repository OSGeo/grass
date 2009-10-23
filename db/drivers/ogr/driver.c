
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*               Some updates by Martin Landa <landa.martin gmail.com>
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004-2009 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

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
