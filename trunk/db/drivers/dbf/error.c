#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "globals.h"

/* init error message */
void init_error(void)
{
    db_d_init_error("DBF");
}
