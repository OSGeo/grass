/*!
  \file lib/db/dbmi_client/handler.c

  \brief DBMI Library (client) - standard error handlers

  (C) 2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/gis.h>
#include <grass/dbmi.h>

static void error_handler_driver(void *p)
{
    dbDriver *driver;

    driver = (dbDriver *) p;
    
    db_close_database(driver);
    db_shutdown_driver(driver);
}

/*!
  \brief Define standard error handler for open database connection

  This handler:
   - close database connection
   - shutdown db driver
  
  Note: It's recommended to call this routine after
  db_start_driver_open_database().

  \param driver DB driver
*/
void db_set_error_handler_driver(dbDriver *driver)
{
    G_add_error_handler(error_handler_driver, driver);
}
