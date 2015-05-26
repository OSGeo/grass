/*!
 * \file table.c
 *
 * \brief Low level drop table function.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Martin Landa <landa.martin gmail.com>
 *
 * \date 2015
 */

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

/*!
 * \brief Low level driver drop table from database.
 *
 * \param name table name to drop
 * \return DB_FAILED on error; DB_OK on success
 */
int db__driver_drop_table(dbString *name)
{
    PGresult *res;
    char cmd[DB_SQL_MAX];

    sprintf(cmd, "DROP TABLE %s", db_get_string(name));
        
    res = PQexec(pg_conn, cmd);

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error("%s\n%s",
			  _("Unable to execute():"),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}
