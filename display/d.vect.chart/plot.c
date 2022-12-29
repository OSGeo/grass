#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include <grass/glocale.h>

#include "global.h"

/* Returns 0 - ok , 1 - error */
int
plot(int ctype, struct Map_info *Map, int type, int field,
     char *columns, int ncols, char *sizecol, int size, double scale,
     COLOR * ocolor, COLOR * colors, int y_center, double *max_reference,
     int do3d)
{
    int ltype, nlines, line, col, more, coltype, nselcols;
    double x, y, csize, len;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int cat;
    double *val;
    char buf[2000];
    struct field_info *Fi;
    dbDriver *driver;
    dbValue *value;
    dbString sql;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    db_init_string(&sql);

    Fi = Vect_get_field(Map, field);
    if (Fi == NULL)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      field);

    /* Open driver */
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL) {
	G_warning(_("Unable to open database <%s> by driver <%s>"),
		  Fi->database,
		  Fi->driver);
	return 1;
    }
    db_set_error_handler_driver(driver);

    val = (double *)G_malloc((ncols + 1) * sizeof(double));	/* + 1 for sizecol */

    Vect_rewind(Map);

    nlines = Vect_get_num_lines(Map);

    /* loop through each vector feature */
    for (line = 1; line <= nlines; line++) {
	G_debug(3, "line = %d", line);
	ltype = Vect_read_line(Map, Points, Cats, line);

	if (!(ltype & type))
	    continue;

	Vect_cat_get(Cats, field, &cat);
	if (cat < 0)
	    continue;

	/* Select values from DB */
	if (ctype == CTYPE_PIE && sizecol != NULL) {
	    sprintf(buf, "select %s, %s from %s where %s = %d", columns,
		    sizecol, Fi->table, Fi->key, cat);
	    nselcols = ncols + 1;
	}
	else {
	    sprintf(buf, "select %s from %s where %s = %d", columns,
		    Fi->table, Fi->key, cat);
	    nselcols = ncols;
	}

	db_set_string(&sql, buf);
	G_debug(3, "SQL: %s", buf);

	if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) !=
	    DB_OK) {
	    G_warning(_("Unable to open select cursor: '%s'"),
		      buf);
	    return 1;
	}

	table = db_get_cursor_table(&cursor);
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK || !more)
	    continue;

	for (col = 0; col < nselcols; col++) {
	    column = db_get_table_column(table, col);
	    value = db_get_column_value(column);
	    coltype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	    switch (coltype) {
	    case DB_C_TYPE_INT:
		val[col] = (double)db_get_value_int(value);
		break;
	    case DB_C_TYPE_DOUBLE:
		val[col] = db_get_value_double(value);
		break;
	    default:
		G_warning("Column type not supported (must be INT or FLOAT)");
		return 1;
	    }
	    G_debug(4, "  val[%d]: %f", col, val[col]);
	}

	db_close_cursor(&cursor);

	/* Center of chart */
	if (ltype & GV_LINES) {	/* find center */
	    len = Vect_line_length(Points) / 2;
	    Vect_point_on_line(Points, len, &x, &y, NULL, NULL, NULL);
	}
	else {
	    x = Points->x[0];
	    y = Points->y[0];
	}

	if (ctype == CTYPE_PIE) {
	    if (sizecol != NULL) {
		csize = val[ncols];
		size = scale * csize;
	    }
	    pie(x, y, size, val, ncols, ocolor, colors, do3d);
	}
	else {
	    bar(x, y, size, scale, val, ncols, ocolor, colors, y_center,
		max_reference, do3d);
	}
    }

    db_close_database_shutdown_driver(driver);
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
