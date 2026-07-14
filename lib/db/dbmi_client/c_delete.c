/*!
 * \file db/dbmi_client/c_delete.c
 *
 * \brief DBMI Library (client) - delete record
 *
 * SPDX-FileCopyrightText:  1999-2008 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>
#include "macros.h"

/*!
   \brief Delete record (?)

   \param cursor db cursor

   \return DB_OK on success
   \return DB_FAILED on failure
 */
int db_delete(dbCursor *cursor)
{
    int ret_code;

    /* start the procedure call */
    db__set_protocol_fds(cursor->driver->send, cursor->driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_DELETE);

    /* send the argument(s) to the procedure */
    DB_SEND_TOKEN(&cursor->token);

    /* get the return code for the procedure call */
    DB_RECV_RETURN_CODE(&ret_code);

    if (ret_code != DB_OK)
        return ret_code; /* ret_code SHOULD == DB_FAILED */

    /* no results */
    return DB_OK;
}
