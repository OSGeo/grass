#define MAX_CHAR_SIZE	1000	/* replace by value from ODBC */
#define DB_MSG 800		/* max length message for report_error() */
#define OD_MSG 500		/* max length of message returned by SQLGetDiagRec() */

/* cursors */
typedef struct _cursor
{
    SQLHSTMT stmt;
    dbToken token;
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int nrows;			/* number of rows selected by SELECT statement */
} cursor;

extern SQLHENV ODenvi;		/* Handle ODBC environment */
extern SQLHDBC ODconn;		/* Handle connection  */
