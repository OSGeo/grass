/*!
 * \file db/dbmi_driver/d_version.c
 *
 * \brief DBMI Library (driver) - version info
 *
 * SPDX-FileCopyrightText: 1999-2008 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
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
