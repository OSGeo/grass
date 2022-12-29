#include <stdlib.h>
#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"
#include "dbdriver.h"

SQLHENV ODenvi;			/* Handle ODBC environment */
SQLHDBC ODconn;			/* Handle connection  */

int main(int argc, char **argv)
{
    init_dbdriver();
    exit(db_driver(argc, argv));

    return 0;
}
