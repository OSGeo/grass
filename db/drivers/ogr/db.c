/*!
  \file db/drivers/db.c
  
  \brief Low level OGR SQL driver
 
  (C) 2004-2009 by the GRASS Development Team
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Some updates by Martin Landa <landa.martin gmail.com>
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "globals.h"
#include "proto.h"

/*!
  \brief Open database (OGR datasource)

  \param handle pointer to dbHandle (db name and schema)

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_open_database(dbHandle * handle)
{
    const char *name;
    dbConnection connection;

    db_get_connection(&connection);
    name = db_get_handle_dbname(handle);

    /* if name is empty use connection.databaseName */
    if (strlen(name) == 0)
	name = connection.databaseName;

    G_debug(3, "db_driver_open_database() name = '%s'", name);

    OGRRegisterAll();

    /* try read-write access */
    hDs = OGROpen(name, TRUE, NULL);
    if (hDs == NULL) {
        /* try read-only access */
        hDs = OGROpen(name, FALSE, NULL);
        if (hDs)
            G_important_message(_("Had to open data source read-only"));
    }

    if (hDs == NULL) {
	db_d_append_error(_("Unable to open OGR data source"));
	db_d_report_error();
	return DB_FAILED;
    }

    G_debug(3, "Datasource opened");

    return DB_OK;
}

/*!
  \brief Close open database

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_close_database()
{
    G_debug(3, "db_driver_close_database()");

    OGR_DS_Destroy(hDs);

    G_debug(3, "Database closed");

    return DB_OK;
}
