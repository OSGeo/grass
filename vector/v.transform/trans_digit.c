#include <math.h>
#include <grass/vector.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "trans.h"

#define PI M_PI

int transform_digit_file(struct Map_info *Old, struct Map_info *New,
                         double ztozero, int swap_xy, int swap_xz,
                         int swap_yz, int swap_after,
                         double *trans_params_def, char **columns, int field)
{
    int i, type, cat, line, ret;
    int verbose, format;
    unsigned int j;
    double *trans_params;
    double ang, x, y, tmp;
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

    driver = NULL;
    fi = NULL;
    if (field > 0) {
	fi = Vect_get_field(Old, field);

	driver = db_start_driver_open_database(fi->driver, fi->database);
	if (!driver)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  fi->database, fi->driver);

	trans_params = (double *)G_calloc(IDX_ZROT + 1, sizeof(double));
    }
    else {
	trans_params = trans_params_def;
	ang = PI * trans_params[IDX_ZROT] / 180;
    }

    line = 0;
    ret = 1;
    format = G_info_format();
    verbose = G_verbose() > G_verbose_min();
    while (TRUE) {
	type = Vect_read_next_line(Old, Points, Cats);

	if (type == -1)	{	/* error */
	    ret = 0;
	    break;
	}

	if (type == -2) {	/* EOF */
	    ret = 1;
	    break;
	}

	if (field != -1 && !Vect_cat_get(Cats, field, NULL))
	    continue;
	
	if (verbose && line % 1000 == 0) {
	    if (format == G_INFO_FORMAT_PLAIN)
		fprintf(stderr, "%d..", line);
	    else
		fprintf(stderr, "%11d\b\b\b\b\b\b\b\b\b\b\b", line);
	}

        if (swap_xy && !swap_after) {
            for (i = 0; i < Points->n_points; i++) {
                x = Points->x[i];
                Points->x[i] = Points->y[i];
                Points->y[i] = x;
            }
        }
        if (swap_xz && !swap_after) {
            for (i = 0; i < Points->n_points; i++) {
                tmp = Points->z[i];
                Points->z[i] = Points->x[i];
                Points->x[i] = tmp;
            }
        }
        if (swap_yz && !swap_after) {
            for (i = 0; i < Points->n_points; i++) {
                tmp = Points->z[i];
                Points->z[i] = Points->y[i];
                Points->y[i] = tmp;
            }
        }

	/* get transformation parameters */
	if (field > 0) {
	    Vect_cat_get(Cats, field, &cat);	/* get first category */
	    if (cat > -1) {
		for (j = 0; j <= IDX_ZROT; j++) {
		    if (columns[j] == NULL) {
			trans_params[j] = trans_params_def[j];
			continue;
		    }
		    ctype = db_column_Ctype(driver, fi->table, columns[j]);
		    switch (ctype) {
		    case DB_C_TYPE_INT:
		    case DB_C_TYPE_DOUBLE:
		    case DB_C_TYPE_STRING:
			break;
		    case -1:
			G_fatal_error(_("Column <%s> not found in table <%s>"),
				      columns[j], fi->table);
		    default:
			G_fatal_error(_("Unsupported column type of <%s>"),
				      columns[j]);
		    }
		    if (db_select_value
			(driver, fi->table, fi->key, cat, columns[j], &val) != 1
			|| db_test_value_isnull(&val)) {
			trans_params[j] = trans_params_def[j];

			G_warning(_("Unable to select value for category %d from table <%s>, column <%s>. "
				   "For category %d using default transformation parameter %.3f."),
				  cat, fi->table, columns[j], cat,
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

	    /* ztozero shifts oldmap z to zero, zshift shifts rescaled object
	     * to target elevation: */
	    Points->z[i] =
		((Points->z[i] + ztozero) * trans_params[IDX_ZSCALE]) +
		trans_params[IDX_ZSHIFT];

            if (swap_after) {
                if (swap_xy) {
                    tmp = Points->x[i];
                    Points->x[i] = Points->y[i];
                    Points->y[i] = tmp;
                }
                if (swap_xz) {
                    tmp = Points->z[i];
                    Points->z[i] = Points->x[i];
                    Points->x[i] = tmp;
                }
                if (swap_yz) {
                    tmp = Points->z[i];
                    Points->z[i] = Points->y[i];
                    Points->y[i] = tmp;
                }
            }
	}

	Vect_write_line(New, type, Points, Cats);
	line++;
    }

    if (verbose && format != G_INFO_FORMAT_PLAIN)
	fprintf(stderr, "\r");
    
    if (field > 0) {
	db_close_database_shutdown_driver(driver);
	G_free((void *)trans_params);
    }
    
    return ret;
}
