
/****************************************************************************
 *
 * MODULE:       execute
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Bernhard Reiter <bernhard intevation.de>, Brad Douglas <rez touchofmadness.com>, Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      ODBC driver
 * COPYRIGHT:    (C) 2000-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <grass/dbmi.h>
#include <stdio.h>
#include <grass/gis.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"


int db__driver_execute_immediate(dbString * sql)
{
    char *s, msg[OD_MSG];
    cursor *c;
    SQLRETURN ret;
    SQLINTEGER err;

    s = db_get_string(sql);

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    ret = SQLExecDirect(c->stmt, s, SQL_NTS);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	SQLGetDiagRec(SQL_HANDLE_STMT, c->stmt, 1, NULL, &err, msg,
		      sizeof(msg), NULL);
	db_d_append_error("SQLExecDirect():\n%s\n%s (%d)\n", s, msg,
			  (int)err);
	db_d_report_error();

	return DB_FAILED;
    }

    free_cursor(c);

    return DB_OK;
}
