#include <grass/gis.h>
#include <grass/vector.h>
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
   \return -1 on error
 */
int trans2d(struct Map_info *In, struct Map_info *Out, int type,
	    double height, const char *field_name, const char *column)
{
    int i, ltype, line, field;
    int cat;
    int ret, ctype;

    struct line_pnts *Points;
    struct line_cats *Cats;

    dbCatValArray cvarr;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    db_CatValArray_init(&cvarr);

    field = Vect_get_field_number(In, field_name);
    
    if (column) {
	struct field_info *Fi;

	dbDriver *driver;

        if (field == -1) {
            G_warning(_("Invalid layer number %d, assuming 1"), field);
            field = 1;
        }

	Fi = Vect_get_field(In, field);
	if (!Fi) {
	    G_warning(_("Database connection not defined for layer <%s>"),
		      field_name);
	    return -1;
	}

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (!driver) {
	    G_warning(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
	    return -1;
	}

	/* column type must numeric */
	ctype = db_column_Ctype(driver, Fi->table, column);
	if (ctype == -1) {
	    G_warning(_("Column <%s> not found in table <%s>"),
		      column, Fi->table);
	    return -1;
	}
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE) {
	    G_warning(_("Column must be numeric"));
	    return -1;
	}

        G_message(_("Fetching height from <%s> column..."), column);
	db_select_CatValArray(driver, Fi->table, Fi->key,
			      column, NULL, &cvarr);

	G_debug(3, "%d records selected", cvarr.n_values);

	db_close_database_shutdown_driver(driver);
    }

    G_message(_("Transforming features..."));
    line = 1;
    while (1) {
	ltype = Vect_read_next_line(In, Points, Cats);
	if (ltype == -1) {
	    G_warning(_("Unable to read vector map"));
	    return -1;
	}
	if (ltype == -2) {	/* EOF */
	    break;
	}

        G_progress(line, 1000);
        
	if (!(ltype & type))
	    continue;

	if (field != -1 && !Vect_cat_get(Cats, field, &cat))
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
    G_progress(1, 1);
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return line - 1;
}
