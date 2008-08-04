#include <grass/dbmi.h>
#include <stdio.h>
#include <grass/gis.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"


int db__driver_close_cursor(dbCursor * dbc)
{
    cursor *c;

    /* get my cursor via the dbc token */
    c = (cursor *) db_find_token(db_get_cursor_token(dbc));
    if (c == NULL)
	return DB_FAILED;

    /* free_cursor(cursor) */
    free_cursor(c);

    return DB_OK;
}


cursor *alloc_cursor()
{
    cursor *c;
    SQLRETURN ret;
    char msg[OD_MSG];
    char *emsg = NULL;
    SQLINTEGER err;

    /* allocate the cursor */
    c = (cursor *) db_malloc(sizeof(cursor));
    if (c == NULL) {
	report_error("allocate cursor");
	return c;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, ODconn, &c->stmt);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	SQLGetDiagRec(SQL_HANDLE_DBC, ODconn, 1, NULL, &err, msg, sizeof(msg),
		      NULL);
	G_asprintf(&emsg, "AllocStatement()\n%s (%d)\n", msg, (int)err);
	report_error(emsg);
	G_free(emsg);

	return c;
    }

    /* tokenize it */
    c->token = db_new_token(c);
    if (c->token < 0) {
	free_cursor(c);
	c = NULL;
	report_error("db_new_token()");
    }

    return c;
}

void free_cursor(cursor * c)
{
    db_drop_token(c->token);
    SQLFreeHandle(SQL_HANDLE_STMT, c->stmt);
    G_free(c);
}
