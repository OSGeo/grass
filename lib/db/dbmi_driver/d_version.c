/*!
 * \file db/dbmi_driver/d_version.c
 * 
 * \brief DBMI Library (driver) - version info
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Get version info

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_d_version(void)
{
    /* no arg(s) */

    /* send the return code */
    DB_SEND_SUCCESS();

    /* send version */
    DB_SEND_C_STRING(DB_VERSION);
    return DB_OK;
}
