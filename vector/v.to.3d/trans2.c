#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
   \brief transform 2d vector features to 3d

   \param In input vector
   \param Out output vector
   \param type feature type to be transformed
   \param height fixed height (used only if column is NULL)
   \param field layer number
   \param column attribute column used for height

   \return number of writen features
 */
int trans2d(struct Map_info *In, struct Map_info *Out, int type,
	    double height, int field, const char *column)
{
    int i, ltype, line;
    int cat;
    int ret, ctype;

    struct line_pnts *Points;
    struct line_cats *Cats;

    dbCatValArray cvarr;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    db_CatValArray_init(&cvarr);

    if (column) {
	struct field_info *Fi;

	dbDriver *driver;

	Fi = Vect_get_field(In, field);
	if (!Fi)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  field);

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (!driver) {
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	}

	/* column type must numeric */
	ctype = db_column_Ctype(driver, Fi->table, column);
	if (ctype == -1)
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  column, Fi->table);
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE) {
	    G_fatal_error(_("Column must be numeric"));
	}

	db_select_CatValArray(driver, Fi->table, Fi->key,
			      column, NULL, &cvarr);

	G_debug(3, "%d records selected", cvarr.n_values);

	db_close_database_shutdown_driver(driver);
    }

    line = 1;
    while (1) {
	ltype = Vect_read_next_line(In, Points, Cats);
	if (ltype == -1) {
	    G_fatal_error(_("Unable to read vector map"));
	}
	if (ltype == -2) {	/* EOF */
	    break;
	}

	if (G_verbose() > G_verbose_min() && (line - 1) % 1000 == 0) {
	    fprintf(stderr, "%7d\b\b\b\b\b\b\b", (line - 1));
	}

	if (!(ltype & type))
	    continue;

	if (column) {
	    Vect_cat_get(Cats, field, &cat);
	    if (cat < 0) {
		G_warning(_("Skipping feature without category"));
		continue;
	    }

	    if (ctype == DB_C_TYPE_DOUBLE)
		ret = db_CatValArray_get_value_double(&cvarr, cat, &height);
	    else {		/* integer */

		int height_i;

		ret = db_CatValArray_get_value_int(&cvarr, cat, &height_i);
		height = (double)height_i;
	    }

	    if (ret != DB_OK)
		G_warning(_("Unable to get height for feature category %d"),
			  cat);
	}

	for (i = 0; i < Points->n_points; i++) {
	    Points->z[i] = height;
	}

	Vect_write_line(Out, ltype, Points, Cats);

	line++;
    }

    if (G_verbose() > G_verbose_min())
	fprintf(stderr, "\r");


    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return line - 1;
}
