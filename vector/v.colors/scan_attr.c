#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

int scan_attr(const struct Map_info *Map, int layer, const char *column_name,
	      double *fmin, double *fmax)
{
    int ctype, is_fp, more, first;
    double fval;
    
    char buf[1024];
    struct field_info *fi;
    dbDriver *driver;
    dbTable *table;
    dbColumn *column;
    dbString stmt;
    dbCursor cursor;
    dbValue *value;
     
    *fmin = *fmax = -1;
    fi = Vect_get_field(Map, layer);
    if (!fi)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      layer);

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);
    
    ctype = db_column_Ctype(driver, fi->table, column_name);
    if (ctype == -1)
	G_fatal_error(_("Column <%s> not found in table <%s>"),
		      column_name, fi->table);
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Column <%s> is not numeric"), column_name);

    is_fp = ctype == DB_C_TYPE_DOUBLE;

    G_snprintf(buf, 1023, "SELECT %s FROM %s", column_name, fi->table);
    G_debug(3, "scan_attr() SQL: %s", buf);
     
    db_init_string(&stmt);
    db_append_string(&stmt, buf);
    
    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error(_("Unable to select data"));
    
    table = db_get_cursor_table(&cursor);
    column = db_get_table_column(table, 0);
    value = db_get_column_value(column);
    
    /* fetch the data */
    first = TRUE;
    while (TRUE) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    return -1;

	if (!more)
	    break;

	if (is_fp)
	    fval = db_get_value_double(value);
	else
	    fval = db_get_value_int(value);

	if (first) {
	    *fmin = *fmax = fval;
	    first = FALSE;
	    continue;
	}

	if (fval <= *fmin)
	    *fmin = fval;
	if (fval >= *fmax)
	    *fmax = fval;
    }

    db_close_database(driver);

    return is_fp;
}
