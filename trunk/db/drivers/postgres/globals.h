#include <libpq-fe.h>

/* cursors */
typedef struct _cursor
{
    PGresult *res;
    int nrows;			/* number of rows in query result */
    int row;			/* current row */
    dbToken token;
    int type;			/* type of cursor: SELECT, UPDATE, INSERT */
    int *cols;			/* indexes of known (type) columns */
    int ncols;			/* number of known columns */
} cursor;

typedef struct
{
    char *host, *port, *options, *tty, *dbname, *user, *password, *schema;
} PGCONN;

/* PostgreSQL data types defined in GRASS
   (see also: /usr/include/postgresql/<version>/server/catalog/pg_type.h)
   PostGIS types are encoded as 17xxx.
   Types/OIDs are fetched in db.c from server.
 */
typedef enum
{				/* name in pg_type, aliases */
    PG_TYPE_UNKNOWN,		/* all types not supported by GRASS */

    PG_TYPE_BIT,		/* bit */
    PG_TYPE_INT2,		/* int2,   smallint */
    PG_TYPE_INT4,		/* int4,   integer, int */
    PG_TYPE_INT8,		/* int8,   bigint */
    PG_TYPE_SERIAL,		/* serial */
    PG_TYPE_OID,		/* oid */

    PG_TYPE_FLOAT4,		/* float4, real */
    PG_TYPE_FLOAT8,		/* float8, double precision */
    PG_TYPE_NUMERIC,		/* numeric, decimal */

    PG_TYPE_CHAR,		/* char,   character */
    PG_TYPE_BPCHAR,		/* ??? blank padded character, oid of this type is returned for char fields */
    PG_TYPE_VARCHAR,		/* varchar,        character varying */
    PG_TYPE_TEXT,		/* text */

    PG_TYPE_DATE,		/* date */
    PG_TYPE_TIME,		/* time */
    PG_TYPE_TIMESTAMP,		/* timestamp */

    PG_TYPE_BOOL,		/* bool, boolean */

    PG_TYPE_POSTGIS_GEOM,	/* geometry column of PostGIS, GRASS internal type */
    PG_TYPE_POSTGIS_TOPOGEOM	/* topogeometry column of PostGIS, GRASS internal type */
} PG_TYPES;


extern PGconn *pg_conn;
extern dbString *errMsg;
extern int (*pg_types)[2];
extern int pg_ntypes;
