/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the
 *            GNU General Public License (>=v2).
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_open_database(dbHandle *handle)
{
    const char *name;
    dbConnection default_connection;
    MYSQL *res;
    dbString sql;

    db_get_connection(&default_connection);
    name = db_get_handle_dbname(handle);

    /* if name is empty use default_connection.databaseName */
    if (strlen(name) == 0)
        name = default_connection.databaseName;

    G_debug(3, "db_driver_open_database() mysql: database definition = '%s'",
            name);

    {
        /* Client version */
        const char *user, *password, *host, *port;
        CONNPAR connpar;

        if (parse_conn(name, &connpar) == DB_FAILED) {
            db_d_report_error();
            return DB_FAILED;
        }

        G_debug(3,
                "host = %s, port = %d, dbname = %s, "
                "user = %s, password = %s",
                connpar.host, connpar.port, connpar.dbname, connpar.user,
                connpar.password);

        db_get_login("mysql", name, &user, &password, &host, &port);

        connection = mysql_init(NULL);
        res =
            mysql_real_connect(connection, host, user, password, connpar.dbname,
                               port != NULL ? atoi(port) : 0, NULL, 0);

        if (res == NULL) {
            db_d_append_error("%s\n%s", _("Connection failed."),
                              mysql_error(connection));
            db_d_report_error();
            return DB_FAILED;
        }

        db_init_string(&sql);
        db_set_string(&sql, "SET SQL_MODE=ANSI_QUOTES;");

        /* Set SQL ANSI_QUOTES MODE which allow to use double quotes instead of
         * backticks */
        if (mysql_query(connection, db_get_string(&sql)) != 0) {
            db_d_append_error("%s %s", _("Unable to set SQL ANSI_QUOTES mode:"),
                              mysql_error(connection));
            db_d_report_error();
            db_free_string(&sql);
            mysql_close(connection);

            return DB_FAILED;
        }
        G_debug(3, "db__driver_open_database(): Set ODBC MySQL DB %s",
                db_get_string(&sql));

        db_free_string(&sql);
    }

    return DB_OK;
}

int db__driver_close_database(void)
{
    mysql_close(connection); /* this will also release connection */

    return DB_OK;
}
