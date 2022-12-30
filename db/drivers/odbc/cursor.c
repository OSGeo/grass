#include <stdio.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"

int db__driver_close_cursor(dbCursor *dbc)
{
    cursor *c;

    /* get my cursor via the dbc token */
    c = (cursor *)db_find_token(db_get_cursor_token(dbc));
    if (c == NULL)
        return DB_FAILED;

    /* free_cursor(cursor) */
    free_cursor(c);

    return DB_OK;
}

<<<<<<< HEAD
cursor *alloc_cursor(void)
=======
cursor *alloc_cursor()
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    cursor *c;
    SQLRETURN ret;
    SQLCHAR msg[OD_MSG];
    SQLINTEGER err;

    /* allocate the cursor */
    c = (cursor *)db_malloc(sizeof(cursor));
    if (c == NULL) {
        db_d_append_error(_("Unable to allocate cursor"));
        db_d_report_error();
        return c;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, ODconn, &c->stmt);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
        SQLGetDiagRec(SQL_HANDLE_DBC, ODconn, 1, NULL, &err, msg, sizeof(msg),
                      NULL);
        db_d_append_error("AllocStatement()\n%s (%d)\n", msg, (int)err);
        db_d_report_error();
        return c;
    }

    /* tokenize it */
    c->token = db_new_token(c);
    if (c->token < 0) {
        free_cursor(c);
        c = NULL;
        db_d_append_error(_("Unable to add new token."));
        db_d_report_error();
    }

    return c;
}

void free_cursor(cursor *c)
{
    db_drop_token(c->token);
    SQLFreeHandle(SQL_HANDLE_STMT, c->stmt);
    G_free(c);
}
