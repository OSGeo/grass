#include "mysql.h"

/* cursors */
typedef struct _cursor
{
    MYSQL_RES *res;
    int nrows;			/* number of rows in query result */
    MYSQL_ROW row;
    dbToken token;
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int *cols;			/* indexes of known (type) columns */
    int ncols;			/* number of known columns */
} cursor;

typedef struct
{
    char *host, *dbname, *user, *password;
    unsigned int port;
} CONNPAR;

extern MYSQL *connection;
extern dbString *errMsg;
