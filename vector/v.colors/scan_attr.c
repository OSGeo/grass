#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

int scan_attr(const struct Map_info *Map, int layer, const char *column_name,
	      const char *style, const char *rules,
	      const struct FPRange *range, struct Colors *colors)
{
    int ctype, is_fp, nrec, i, cat;
    int red, grn, blu;
    double fmin, fmax;
    
    struct field_info *fi;
    struct Colors vcolors;
    dbDriver *driver;
    dbCatVal *cv;
    dbCatValArray cvarr;

    Rast_init_colors(colors);
    
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

    nrec = db_select_CatValArray(driver, fi->table, fi->key, column_name,
				 NULL, &cvarr);
    if (nrec < 1) {
	G_important_message(_("No data selected"));
	return 0;
    }
    
    /* color table for values */
    db_CatValArray_sort_by_value(&cvarr);
    if (is_fp) {
	fmin = cvarr.value[0].val.d;
	fmax = cvarr.value[cvarr.n_values-1].val.d;

	if (range) {
	    if (range->min >= fmin && range->min <= fmax)
		fmin = range->min;
	    else
		G_warning(_("Min value (%f) is out of range %f,%f"),
			  range->min, fmin, fmax);
	    
	    if (range->max <= fmax && range->max >= fmin)
		fmax = range->max;
	    else
		G_warning(_("Max value (%f) is out of range %f,%f"),
			  range->max, fmin, fmax);
	}
    }
    else {
	fmin = cvarr.value[0].val.i;
	fmax = cvarr.value[cvarr.n_values-1].val.i;
	
	if (range) {
	    if (range->min >= fmin && range->min <= fmax)
		fmin = range->min;
	    else
		G_warning(_("Min value (%d) is out of range %d,%d"),
			  (int) range->min, (int) fmin, (int) fmax);
	    
	    if (range->max <= fmax && range->max >= fmin)
		fmax = range->max;
	    else
		G_warning(_("Max value (%d) is out of range %d,%d"),
			  (int) range->max, (int) fmin, (int) fmax);
	}
    }
    if (style)
	make_colors(&vcolors, style, (DCELL) fmin, (DCELL) fmax, is_fp);
    else if (rules)
	load_colors(&vcolors, rules, (DCELL) fmin, (DCELL) fmax, is_fp);

    /* color table for categories */
    for (i = 0; i < cvarr.n_values; i++) {
	cv = &(cvarr.value[i]);
	cat = cv->cat;
	if (is_fp) {
	    if (Rast_get_d_color((const DCELL *) &(cv->val.d), &red, &grn, &blu,
				 &vcolors) == 0) {
		/* G_warning(_("No color rule defined for value %f"), cv->val.d); */
		G_debug(3, "scan_attr(): cat=%d, val=%f -> no color rule", cat, cv->val.d);
		continue;
	    }
	}
	else {
	    if (Rast_get_c_color((const CELL *) &(cv->val.i), &red, &grn, &blu,
				 &vcolors) == 0) {
		/* G_warning(_("No color rule defined for value %d"), cv->val.i); */
		G_debug(3, "scan_attr(): cat=%d, val=%d -> no color rule", cat, cv->val.i);
		continue;
	    }
	}
	G_debug(3, "scan_attr(): cat=%d, val=%f, r=%d, g=%d, b=%d",
		cat, is_fp ? cv->val.d : cv->val.i, red, grn, blu);
	Rast_add_c_color_rule((const CELL*) &cat, red, grn, blu,
			      (const CELL*) &cat, red, grn, blu, colors);
    }
    
    db_close_database(driver);

    return is_fp;
}
