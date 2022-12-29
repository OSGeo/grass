#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "plot.h"

int display_attr(struct Map_info *Map, int type, char *attrcol,
		 struct cat_list *Clist, LATTR *lattr, int chcat)
{
    int i, ltype, more;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int cat;
    char buf[2000];
    struct field_info *fi;
    dbDriver *driver;
    dbString stmt, valstr, text;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;

    G_debug(2, "attr()");

    if (attrcol == NULL || *attrcol == '\0') {
	G_fatal_error(_("attrcol not specified, cannot display attributes"));
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    db_init_string(&stmt);
    db_init_string(&valstr);
    db_init_string(&text);

    fi = Vect_get_field(Map, lattr->field);
    if (fi == NULL)
	return 1;

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database,
		      fi->driver);

    Vect_rewind(Map);
    while (1) {
	ltype = Vect_read_next_line(Map, Points, Cats);
	if (ltype == -1)
	    G_fatal_error(_("Unable to read vector map"));
	else if (ltype == -2)		/* EOF */
	    break;

	if (!(type & ltype) && !((type & GV_AREA) && (ltype & GV_CENTROID)))
	    continue;		/* used for both lines and labels */

	D_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B);
	D_text_size(lattr->size, lattr->size);
	if (lattr->font)
	    D_font(lattr->font);
	if (lattr->enc)
	    D_encoding(lattr->enc);

	if (chcat) {
	    int found = 0;

	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == Clist->field &&
		    Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
		    found = 1;
		    break;
		}
	    }
	    if (!found)
		continue;
	}
	else if (Clist->field > 0) {
	    int found = 0;

	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == Clist->field) {
		    found = 1;
		    break;
		}
	    }
	    /* lines with no category will be displayed */
	    if (Cats->n_cats > 0 && !found)
		continue;
	}

	if (Vect_cat_get(Cats, lattr->field, &cat)) {
	    int ncats = 0;

	    /* Read attribute from db */
	    db_free_string(&text);
	    for (i = 0; i < Cats->n_cats; i++) {
		int nrows;

		if (Cats->field[i] != lattr->field)
		    continue;
		db_init_string(&stmt);
		sprintf(buf, "select %s from %s where %s = %d", attrcol,
			fi->table, fi->key, Cats->cat[i]);
		G_debug(2, "SQL: %s", buf);
		db_append_string(&stmt, buf);

		if (db_open_select_cursor
		    (driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
		    G_fatal_error(_("Unable to open select cursor: '%s'"),
				  db_get_string(&stmt));

		nrows = db_get_num_rows(&cursor);

		if (ncats > 0)
		    db_append_string(&text, "/");

		if (nrows > 0) {
		    table = db_get_cursor_table(&cursor);
		    column = db_get_table_column(table, 0);	/* first column */

		    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
			continue;
		    db_convert_column_value_to_string(column, &valstr);

		    db_append_string(&text, db_get_string(&valstr));
		}
		else {
		    G_warning(_("No attribute found for cat %d: %s"), cat,
			      db_get_string(&stmt));
		}

		db_close_cursor(&cursor);
		ncats++;
	    }

	    show_label_line(Points, ltype, lattr, db_get_string(&text));
	}
    }

    db_close_database_shutdown_driver(driver);
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
