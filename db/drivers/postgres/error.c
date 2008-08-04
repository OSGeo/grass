#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "globals.h"

/* init error message */
void init_error(void)
{
    if (!errMsg) {
	errMsg = (dbString *) G_malloc(sizeof(dbString));
	db_init_string(errMsg);
    }

    db_set_string(errMsg, "DBMI-Postgres driver error:\n");
}

/* append error message */
void append_error(const char *msg)
{
    db_append_string(errMsg, msg);
}


void report_error(void)
{
    db_append_string(errMsg, "\n");
    db_error(db_get_string(errMsg));
}
