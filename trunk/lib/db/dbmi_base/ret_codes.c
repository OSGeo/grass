/*!
  \file lib/db/dbmi_base/ret_codes.c
  
  \brief DBMI Library (base) - return codes (internal use only)

  \todo Are we as restrictive here as for vector names?
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <stdlib.h>
#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Send success code

  \return DB_OK
*/
int db__send_success()
{
    DB_SEND_INT(DB_OK);
    return DB_OK;
}

/*!
  \brief Send failure code

  \return DB_OK
*/
int db__send_failure()
{
    DB_SEND_INT(DB_FAILED);
    DB_SEND_C_STRING(db_get_error_msg());
    return DB_OK;
}

/*!
  \brief Receive return code
  
  \param[out] ret_code return code

  \return DB_OK on success
*/
int db__recv_return_code(int *ret_code)
{
    dbString err_msg;

    /* get the return code first */
    DB_RECV_INT(ret_code);

    /* if OK, we're done here */
    if (*ret_code == DB_OK)
	return DB_OK;

    /* should be DB_FAILED */
    if (*ret_code != DB_FAILED) {
	db_protocol_error();
	return DB_PROTOCOL_ERR;
    }
    /* get error message from driver */
    db_init_string(&err_msg);
    DB_RECV_STRING(&err_msg);

    db_error(db_get_string(&err_msg));
    db_free_string(&err_msg);

    return DB_OK;
}
