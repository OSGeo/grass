#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include "proto.h"

/* 1/2001 added field parameter MN
 * update 12/99 to read multi-dim sites properly MN
 * updated 28 June 1995 to use new sites API.
 * Only uses floating point attributes. 
 * mccauley
 */

void read_sites(const char *name, const char *field_name, const char *col, int noindex)
{
    extern long npoints;
    int nrec, ctype = 0, type, field, with_z;
    struct Map_info Map;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;
    struct line_pnts *Points;
    struct line_cats *Cats;

    Vect_set_open_level(1);	/* without topology */
    if (Vect_open_old2(&Map, name, "", field_name) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), name);

    field = Vect_get_field_number(&Map, field_name);
    with_z = col == NULL && Vect_is_3d(&Map); /* read z-coordinates
                                                 only when column is
                                                 not defined */
    if (!col) {
        if (!with_z)
            G_important_message(_("Input vector map <%s> is 2D - using categories to interpolate"),
                                Vect_get_full_name(&Map));
        else
            G_important_message(_("Input vector map <%s> is 3D - using z-coordinates to interpolate"),
                                Vect_get_full_name(&Map));

    }

    if (col) {
	db_CatValArray_init(&cvarr);

	Fi = Vect_get_field(&Map, field);
	if (Fi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %s"),
			  field_name);

	Driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (Driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

	nrec =
	    db_select_CatValArray(Driver, Fi->table, Fi->key, col, NULL,
				  &cvarr);
	G_debug(3, "nrec = %d", nrec);

	ctype = cvarr.ctype;
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Column type not supported"));

	if (nrec < 0)
	    G_fatal_error(_("Unable to select data from table"));

	G_verbose_message(n_("One record selected from table", "%d records selected from table", nrec), nrec);

	db_close_database_shutdown_driver(Driver);
    }


    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    while ((type = Vect_read_next_line(&Map, Points, Cats)) >= 0) {
	double dval;

	if (!(type & GV_POINTS))
	    continue;

	if (!with_z) {
	    int cat, ival, ret;

	    /* TODO: what to do with multiple cats */
	    Vect_cat_get(Cats, field, &cat);
	    if (cat < 0) /* skip features without category */
		continue;

            if (col) {
                if (ctype == DB_C_TYPE_INT) {
                    ret = db_CatValArray_get_value_int(&cvarr, cat, &ival);
                    dval = ival;
                }
                else {		/* DB_C_TYPE_DOUBLE */
                    ret = db_CatValArray_get_value_double(&cvarr, cat, &dval);
                }
            }
            else {
                dval = cat;
            }

	    if (ret != DB_OK) {
		G_warning(_("No record for point (cat = %d)"), cat);
		continue;
	    }
	}
	else
	    dval = Points->z[0];

	newpoint(dval, Points->x[0], Points->y[0], noindex);
    }

    if (col)
	db_CatValArray_free(&cvarr);

    Vect_set_release_support(&Map);
    Vect_close(&Map);

    G_message(n_("%ld point loaded", "%ld points loaded", npoints), npoints);
}
