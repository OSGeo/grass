#define MAIN
#include <stdlib.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "dbdriver.h"

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}
