#include <grass/sqlp.h>

#define DBF_COL_NAME 11		/* maximum column name (10 characters plus \0) */

/* 
 * DBF API:      http://shapelib.maptools.org/dbf_api.html
 *            or ../../../lib/external/shapelib/shapefil.h
 *
 * DBFFieldType: FTString, FTInteger, FTDouble, FTLogical, FTInvalid
 *                  0          1          2         4         5
 *                DBF_CHAR   DBF_INT   DBF_DOUBLE
 *                  1          2          3
 */
#define DBF_CHAR   1
#define DBF_INT    2
#define DBF_DOUBLE 3

typedef struct
{
    char name[DBF_COL_NAME];
    int type;
    int width;
    int decimals;
} COLUMN;

typedef struct
{
    char *c;
    int i;
    double d;
    int is_null;
} VALUE;

typedef struct
{
    int alive;
    VALUE *values;
} ROW;

typedef struct
{
    char name[1024];		/* table name (without .dbf) */
    char file[1024];		/* full path to file (including .dbf) */
    int read;			/* TRUE if user has read access to the file */
    int write;			/* TRUE if user has write access to the file */
    int alive;
    int described;		/* columns definitions were loaded to heads */
    int loaded;			/* data were loaded to rows */
    int updated;
    COLUMN *cols;
    ROW *rows;
    int acols;			/* allocated columns */
    int ncols;			/* number of columns */
    int arows;			/* allocated rows */
    int nrows;			/* number of rows */
} TABLE;

typedef struct
{
    char name[1024];		/* db name = full path to db dir */
    TABLE *tables;
    int atables;		/* allocated space for tables */
    int ntables;		/* number of tables */
} DATABASE;

/* cursors */
typedef struct
{
    SQLPSTMT *st;
    int table;			/* table */
    int *set;			/* array of indexes to table for selected rows */
    int nrows;			/* number of rows in set */
    int cur;			/* position of cursor */
    int *cols;			/* array of indexes of selected columns */
    int ncols;
    dbToken token;
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int *order;			/* array of row indexes (sorted by ORDER BY) */
} cursor;

extern DATABASE db;
extern dbString *errMsg;
