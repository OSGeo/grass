#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

struct Colors *make_colors(const char *name, const char *layer,
			   const char *column, struct Colors *cat_colors)
{
    int field, is_fp, ctype, nrec, cat, i;
    int red, grn, blu;
    
    struct Map_info Map;
    struct field_info *fi;
    struct Colors *colors;
    
    dbDriver *driver;
    dbCatValArray cvarr;
    dbCatVal *cv;
    
    Vect_set_open_level(1); /* no topology required */
    if (Vect_open_old2(&Map, name, "", layer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), name);

    field = Vect_get_field_number(&Map, layer);
    if (field < 1)
	G_fatal_error(_("Layer <%s> not found"), layer);

    fi = Vect_get_field(&Map, field);
    if (!fi)
	G_fatal_error(_("Database connection not defined for layer <%s>"),
		      layer);

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);
    db_set_error_handler_driver(driver);

    ctype = db_column_Ctype(driver, fi->table, column);
    if (ctype == -1)
	G_fatal_error(_("Column <%s> not found in table <%s>"),
		      column, fi->table);
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Column <%s> is not numeric"), column);

    is_fp = ctype == DB_C_TYPE_DOUBLE;

    nrec = db_select_CatValArray(driver, fi->table, fi->key, column,
				 NULL, &cvarr);
    if (nrec < 1) {
	G_important_message(_("No data selected"));
	return 0;
    }
    
    colors = (struct Colors *) G_malloc(sizeof(struct Colors));
    Rast_init_colors(colors);

    for (i = 0; i < cvarr.n_values; i++) {
	cv = &(cvarr.value[i]);
	cat = cv->cat;
	if (Rast_get_c_color((const CELL *) &cat, &red, &grn, &blu,
			     cat_colors) == 0)
	    continue;
	
	if (is_fp)
	    Rast_add_d_color_rule((const DCELL*) &(cv->val.d), red, grn, blu,
				  (const DCELL*) &(cv->val.d), red, grn, blu,
				  colors);
	else
	    Rast_add_c_color_rule((const CELL*) &(cv->val.i), red, grn, blu,
				  (const CELL*) &(cv->val.i), red, grn, blu,
				  colors);
	
    }
    
    Vect_close(&Map);
    
    return colors;
}
