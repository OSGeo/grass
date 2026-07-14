/*!
 * \file db/dbmi_client/c_list_drivers.c
 *
 * \brief DBMI Library (client) - list drivers
 *
 * SPDX-FileCopyrightText:  1999-2008 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>

/*!
   \brief Return comma separated list of existing DB drivers, used for driver
   parameter options

   \return list of db drivers
 */
const char *db_list_drivers(void)
{
    dbDbmscap *list, *cur;
    dbString drivernames;

    db_init_string(&drivernames);

    /* read the dbmscap info */
    if (NULL == (list = db_read_dbmscap()))
        return NULL;
    else {
        /* build the comma separated string of existing drivers */
        for (cur = list; cur; cur = cur->next) {
            if (cur->driverName[0] == '\0')
                break;
            else {
                if (cur != list)
                    db_append_string(&drivernames, ",");
                db_append_string(&drivernames, cur->driverName);
            }
        }
    }

    db_free_dbmscap(list);
    return db_get_string(&drivernames);
}
