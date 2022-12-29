#include <stdlib.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "dbdriver.h"

PGconn *pg_conn;		/* Database connection */
int (*pg_types)[2] = NULL;	/* array of types, first is internal code, second PG_TYPE_* */
int pg_ntypes = 0;
dbString *errMsg = NULL;	/* error message */

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}
