#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"

int db__driver_create_table(dbTable * table)
{
    dbString sql;
    cursor *c;
    char msg[OD_MSG];
    char *emsg = NULL;
    SQLRETURN ret;
    SQLINTEGER err;

    G_debug(3, "db__driver_create_table()");

    db_init_string(&sql);
    db_table_to_sql(table, &sql);

    G_debug(3, " SQL: %s", db_get_string(&sql));

    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    ret = SQLExecDirect(c->stmt, db_get_string(&sql), SQL_NTS);

    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	SQLGetDiagRec(SQL_HANDLE_STMT, c->stmt, 1, NULL, &err, msg,
		      sizeof(msg), NULL);
	db_d_append_error("SQLExecDirect():\n%s\n%s (%d)\n",
			  db_get_string(&sql), msg, (int)err);
	db_d_report_error();
	G_free(emsg);

	return DB_FAILED;
    }

    free_cursor(c);

    return DB_OK;
}
