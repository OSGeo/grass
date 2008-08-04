#include <grass/gis.h>
#include "pg.h"
#include <stdio.h>


char *buildPg(char *ktab, char *keycat, int curcat)
{


    static char SQL_stmt[QRY_LENGTH];


    sprintf(SQL_stmt, "SELECT * from %s where %s=%d", ktab, keycat, curcat);

    return (SQL_stmt);

}

char *buildPgSite(char *ktab, char *ycol, char *xcol)
{
    static char SQL_stmt[QRY_LENGTH];

    sprintf(SQL_stmt,
	    "SELECT * from %s where point(%s,%s) ", ktab, xcol, ycol);

    return (SQL_stmt);

}
