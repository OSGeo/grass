#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/Vect.h>
#include "pg.h"



char *query_postgr(name, keytable, col, x, y)

     char *name, *keytable, *col;
     float x, y;

#if defined(HAVE_LIBPQ_FE_H)

{

    char *openvect();
    int level;
    static char buf[32];
    struct Map_info P_map;

    char *SQL_stmt;
    static char long_str[2 * QRY_LENGTH];
    char *qry_str;

    int dbCat;
    char *mapset, *dbname;


    memset(buf, '\0', sizeof(buf));
    memset(long_str, '\0', sizeof(long_str));

    /* Check DATABASE env variable */
    if ((dbname = G__getenv("PG_DBASE")) == NULL) {
	sprintf(buf, "Please run g.select.pg first\n");
	return buf;
    }


    if ((mapset = openvect(name)) == NULL) {
	sprintf(buf, "Unable to open %s\n", name);
	return buf;
    }


    level = Vect_open_old(&P_map, name, mapset);
    if (level < 0)
	G_fatal_error("Can't open vector map");
    if (level < 2)
	G_fatal_error("You must first run v.support on vector map");



    qry_str = (char *)getCat(&P_map, x, y, &dbCat);
    sprintf(long_str, "%s", qry_str);
    if (dbCat > 0) {
	SQL_stmt = (char *)buildPg(keytable, col, dbCat);
	qry_str = (char *)runPg(SQL_stmt);
	if (qry_str != NULL)
	    strncat(long_str, qry_str, QRY_LENGTH);
	else {
	    qry_str = "\nThere's been ERROR from Postgres\n";
	    strncat(long_str, qry_str, QRY_LENGTH);
	}
    }

    Vect_close(&P_map);
    return long_str;

}
#else
{



    static char long_str[128] =
	"Postgres support had not been enabled during pre-compile.\nYou should recompile NVIZ with Postgres support.\n";



    return long_str;

}

#endif
char *query_pg_site(name, xcol, ycol, dist, x, y)

     char *name, *xcol, *ycol;
     int dist;
     float x, y;

#if defined(HAVE_LIBPQ_FE_H)

{

    char *dbname;
    static char buf[32];
    char *SQL_stmt;
    static char long_str[2 * QRY_LENGTH];
    char *qry_str;
    struct Sql *pts;
    int ret;

    memset(buf, '\0', sizeof(buf));
    memset(long_str, '\0', sizeof(long_str));

    /* Check DATABASE env variable */
    if ((dbname = G__getenv("PG_DBASE")) == NULL) {
	sprintf(buf, "Please run g.select.pg first\n");
	return buf;
    }
    SQL_stmt = buildPgSite(name, ycol, xcol);

    /* Initialze SQL query structure        */
    pts = (struct Sql *)G_malloc(sizeof(struct Sql));
    G_zero(pts, sizeof(struct Sql));
    ret = fillSQLstruct(pts, x, y, dist);

    qry_str = do_query(SQL_stmt, pts);

    if (qry_str != NULL)
	strncat(long_str, qry_str, QRY_LENGTH);
    else {
	qry_str = "\nThere's been ERROR from Postgres\n";
	strncat(long_str, qry_str, QRY_LENGTH);
    }
    return long_str;
}
#else
{

    static char long_str[128] =
	"Postgres support had not been enabled during pre-compile.\nYou should recompile NVIZ with Postgres support.\n";



    return long_str;
}

#endif
