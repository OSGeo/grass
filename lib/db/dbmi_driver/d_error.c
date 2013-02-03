/*!
  \file lib/db/dbmi_driver/d_error.c
  
  \brief DBMI Library (driver) - error reporting
  
  Taken from DB drivers.
  
  (C) 1999-2008, 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC)
  \author Radim Blazek
  \author Adopted for DBMI by Martin Landa <landa.martin@gmail.com>
*/

#include <string.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* initialize the global struct */
struct error_state {
    char     *driver_name;
    dbString *errMsg;
};

static struct error_state state;
static struct error_state *st = &state;

static void init()
{
    db_set_string(st->errMsg, "");
    db_d_append_error(_("DBMI-%s driver error:"), st->driver_name);
    db_append_string(st->errMsg, "\n");
}


/*!
  \brief Init error message for DB driver
  
  Initialize prefix

  \param name driver name (eg. "SQLite"))
*/
void db_d_init_error(const char *name)
{
    if (!st->errMsg) {
	st->errMsg = (dbString *) G_malloc(sizeof(dbString));
	db_init_string(st->errMsg);
    }
    
    G_debug(1, "db_d_init_error(): %s", name);
    
    st->driver_name = G_malloc(strlen(name) + 1);
    strcpy(st->driver_name, name);
    init();
}

/*!
  \brief Append error message for DB driver

  \param fmt formatted message
*/
void db_d_append_error(const char *fmt, ...)
{
    FILE *fp = NULL;
    char *work = NULL;
    int count = 0;
    va_list ap;

    va_start(ap, fmt);
    if ((fp = tmpfile())) {
	count = vfprintf(fp, fmt, ap);
	if (count >= 0 && (work = G_calloc(count + 1, 1))) {
	    rewind(fp);
	    fread(work, 1, count, fp);
	    db_append_string(st->errMsg, work);
	    G_free(work);
	}
	fclose(fp);
    }
    va_end(ap);
}

/*!
  \brief Report error message for DB driver
*/
void db_d_report_error(void)
{
    db_append_string(st->errMsg, "\n");
    db_error(db_get_string(st->errMsg));

    init();
}
