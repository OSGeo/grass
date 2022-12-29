/*!
  \file db/dbmi_client/c_openselect.c
  
  \brief DBMI Library (client) - open select cursor
  
  (C) 1999-2008, 2012 by the GRASS Development Team
 
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC)
  \author Radim Blazek
 */

#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Open select cursor

  Open modes:
   - DB_SEQUENTIAL

  Data can be fetched by db_fetch().
  
  Cursor should be closed by db_close_cursor().
  
  \param driver pointer to dbDriver
  \param select SQL select statement (pointer to dbString)
  \param cursor pointer to dbCursor to be opened
  \param mode open mode

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_open_select_cursor(dbDriver * driver, dbString * select, dbCursor * cursor,
			  int mode)
{
    int ret_code;

    db_init_cursor(cursor);
    cursor->driver = driver;

    /* start the procedure call */
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_OPEN_SELECT_CURSOR);

    /* send the argument(s) to the procedure */
    DB_SEND_STRING(select);
    DB_SEND_INT(mode);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
	return ret_code;	/* ret_code SHOULD == DB_FAILED */

    /* get the results */
    DB_RECV_TOKEN(&cursor->token);
    DB_RECV_INT(&cursor->type);
    DB_RECV_INT(&cursor->mode);
    DB_RECV_TABLE_DEFINITION(&cursor->table);
    return DB_OK;
}
