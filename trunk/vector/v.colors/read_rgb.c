#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include <grass/glocale.h>

void rgb2colr(const struct Map_info *Map, int layer, const char *rgb_column,
	      struct Colors *colors)
{
    int i, ret, nskipped;
    int red, grn, blu;
    const char *rgb;
    
    struct field_info *fi;

    dbDriver *driver;
    dbCatValArray cvarr;
    dbCatVal *cv;

    fi = Vect_get_field(Map, layer);
    if (!fi)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      layer);

    driver = db_start_driver_open_database(fi->driver, fi->database);
    if (!driver)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      fi->database, fi->driver);

    if (db_column_Ctype(driver, fi->table, rgb_column) != DB_C_TYPE_STRING)	
	G_fatal_error(_("Data type of RGB column <%s> must be char"),
		      rgb_column);
    
    if (0 > db_select_CatValArray(driver, fi->table, fi->key,
				  rgb_column, NULL, &cvarr))
	G_warning(_("No RGB values found"));

    Rast_init_colors(colors);
    
    cv = NULL;
    nskipped = 0;
    for (i = 0; i < cvarr.n_values; i++) {
	cv = &(cvarr.value[i]);
	rgb = db_get_string(cv->val.s);
	G_debug(3, "cat = %d RGB = %s", cv->cat, rgb);

        if (!rgb) {
            nskipped++;
            continue;
        }
        
	ret = G_str_to_color(rgb, &red, &grn, &blu);
	if (ret != 1) {
	    G_debug(3, "Invalid RGB value '%s'", rgb);
            nskipped++;
	    continue;
	}

	Rast_add_c_color_rule((const CELL*) &(cv->cat), red, grn, blu,
			      (const CELL*) &(cv->cat), red, grn, blu, colors);
    }

    if (nskipped > 0)
        G_warning(_("%d invalid RGB color values skipped"), nskipped);
    
    db_close_database_shutdown_driver(driver);
}
