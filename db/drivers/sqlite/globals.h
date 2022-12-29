#include <sqlite3.h>

#ifndef DBMI_SQLITE_PROTO_H
#define DBMI_SQLITE_PROTO_H

/* cursors */
typedef struct _cursor
{
    sqlite3_stmt *statement;
    int nrows;			/* number of rows in query result, -1 if unknown */
    int row;			/* current row */
    dbToken token;
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int *kcols;			/* indexes of known (type) columns */
    int nkcols;			/* number of known columns */

} cursor;

extern sqlite3 *sqlite;

#endif
