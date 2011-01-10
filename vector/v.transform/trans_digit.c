/* **************************************************************************
 *
 * MODULE:       v.transform
 * AUTHOR(S):    See other files as well...
 *               Eric G. Miller <egm2@jps.net>
 *               Radim Blazek
 *               DB support added by Martin Landa (08/2007)
 *
 * PURPOSE:      To transform a vector layer's coordinates via a set of tie
 *               points.
 *
 * COPYRIGHT:    (C) 2002-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

#include <math.h>
#include <grass/libtrans.h>
#include <grass/vector.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "trans.h"

#define PI M_PI

int
transform_digit_file(struct Map_info *Old, struct Map_info *New,
		     int shift_file, double ztozero, int swap, double *trans_params_def,
		     char *table, char **columns, int field)
{
    int i, type, cat;
    unsigned int j;
    double *trans_params;
    double ang, x, y;
    static struct line_pnts *Points;
    static struct line_cats *Cats;

    /* db */
    struct field_info *fi;
    int ctype;
    dbDriver *driver;
    dbValue val;

    cat = -1;			/* dummy value for debugging */

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if (table) {
	fi = Vect_default_field_info(Old, 1, NULL, GV_1TABLE);

	driver = db_start_driver_open_database(fi->driver, fi->database);
	if (!driver)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  fi->database, fi->driver);

	trans_params = (double *)G_calloc(IDX_ZROT, sizeof(double));
    }
    else {
	trans_params = trans_params_def;
	ang = PI * trans_params[IDX_ZROT] / 180;
    }

    while (1) {
	type = Vect_read_next_line(Old, Points, Cats);

	if (type == -1)		/* error */
	    return 0;

	if (type == -2)		/* EOF */
	    return 1;

	if (field != -1 && !Vect_cat_get(Cats, field, NULL))
	    continue;
	
	if (swap) {
	    for (i = 0; i < Points->n_points; i++) {
		x = Points->x[i];
		Points->x[i] = Points->y[i];
		Points->y[i] = x;
	    }
	} 
	
	/* get transformation parameters */
	if (table) {
	    Vect_cat_get(Cats, field, &cat);	/* get first category */
	    if (cat > -1) {
		for (j = 0; j <= IDX_ZROT; j++) {
		    if (columns[j] == NULL) {
			trans_params[j] = trans_params_def[j];
			continue;
		    }
		    ctype = db_column_Ctype(driver, table, columns[j]);
		    switch (ctype) {
		    case DB_C_TYPE_INT:
		    case DB_C_TYPE_DOUBLE:
		    case DB_C_TYPE_STRING:
			break;
		    case -1:
			G_fatal_error(_("Missing column <%s> in table <%s>"),
				      columns[j], table);
		    default:
			G_fatal_error(_("Unsupported column type of <%s>"),
				      columns[j]);
		    }
		    if (db_select_value
			(driver, table, fi->key, cat, columns[j], &val) != 1
			|| db_test_value_isnull(&val)) {
			trans_params[j] = trans_params_def[j];

			G_warning(_("Unable to select value for category %d from table <%s>, column <%s>. "
				   "For category %d using default transformation parameter %.3f."),
				  cat, table, columns[j], cat,
				  trans_params[j]);
		    }
		    else {
			trans_params[j] = db_get_value_as_double(&val, ctype);
		    }
		}
	    }
	    else {
		G_warning(_("No category number defined. Using default transformation parameters."));

		for (j = 0; j <= IDX_ZROT; j++) {
		    trans_params[j] = trans_params_def[j];
		}
	    }
	    ang = PI * trans_params[IDX_ZROT] / 180;
	}

	/* transform points */
	for (i = 0; i < Points->n_points; i++) {
	    if (shift_file) {
		transform_a_into_b(Points->x[i], Points->y[i],
				   &(Points->x[i]), &(Points->y[i]));
	    }
	    else {
		G_debug(3, "idx=%d, cat=%d, xshift=%g, yshift=%g, zshift=%g, "
			"xscale=%g, yscale=%g, zscale=%g, zrot=%g",
			i, cat, trans_params[IDX_XSHIFT],
			trans_params[IDX_YSHIFT], trans_params[IDX_ZSHIFT],
			trans_params[IDX_XSCALE], trans_params[IDX_YSCALE],
			trans_params[IDX_ZSCALE], trans_params[IDX_ZROT]);

		/* transform point */
		x = trans_params[IDX_XSHIFT] +
		    trans_params[IDX_XSCALE] * Points->x[i] * cos(ang)
		    - trans_params[IDX_YSCALE] * Points->y[i] * sin(ang);
		y = trans_params[IDX_YSHIFT] +
		    trans_params[IDX_XSCALE] * Points->x[i] * sin(ang)
		    + trans_params[IDX_YSCALE] * Points->y[i] * cos(ang);
		Points->x[i] = x;
		Points->y[i] = y;
	    }

	    /* ztozero shifts oldmap z to zero, zshift shifts rescaled object
	     * to target elevation: */
	    Points->z[i] =
		((Points->z[i] + ztozero) * trans_params[IDX_ZSCALE]) +
		trans_params[IDX_ZSHIFT];
	}

	Vect_write_line(New, type, Points, Cats);
    }

    if (field > 0) {
	db_close_database_shutdown_driver(driver);
	G_free((void *)trans_params);
    }
}
