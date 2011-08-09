#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

int display_shape(struct Map_info *Map, int type, struct cat_list *Clist, const struct Cell_head *window, 
		  const struct color_rgb *bcolor, const struct color_rgb *fcolor, int chcat,
		  const char *icon, double size, const char *size_column, int sqrt_flag, const char *rot_column, /* lines only */
		  int id_flag, int table_colors_flag, int cats_colors_flag, char *rgb_column,
		  int default_width, char *width_column, double width_scale,
		  int z_color_flag, char *z_column)
{
    int open_db, field, i, stat;
    dbCatValArray cvarr_rgb, cvarr_width, cvarr_size, cvarr_rot;
    struct field_info *fi;
    dbDriver *driver;
    int nrec_rgb, nrec_width, nrec_size, nrec_rot;

    stat = 0;
    nrec_rgb = nrec_width = nrec_size = nrec_rot = 0;
    
    open_db = table_colors_flag || width_column || size_column || rot_column;
    if (open_db) {
	field = Clist->field > 0 ? Clist->field : 1;
	fi = Vect_get_field(Map, field);
	if (!fi) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  field);
	}
	
	driver = db_start_driver_open_database(fi->driver, fi->database);
	if (!driver)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  fi->database, fi->driver);
    }
    
    if (table_colors_flag) {
	/* for reading RRR:GGG:BBB color strings from table */
	if (!rgb_column || *rgb_column == '\0')
	    G_fatal_error(_("Color definition column not specified"));
	
	db_CatValArray_init(&cvarr_rgb);

	nrec_rgb = db_select_CatValArray(driver, fi->table, fi->key,
					 rgb_column, NULL, &cvarr_rgb);

	G_debug(3, "nrec_rgb (%s) = %d", rgb_column, nrec_rgb);

	if (cvarr_rgb.ctype != DB_C_TYPE_STRING)
	    G_fatal_error(_("Color definition column (%s) not a string. "
			    "Column must be of form RRR:GGG:BBB where RGB values range 0-255."),
			  rgb_column);


	if (nrec_rgb < 0)
	    G_fatal_error(_("Unable to select data (%s) from table"),
			  rgb_column);

	G_debug(2, "\n%d records selected from table", nrec_rgb);

	for (i = 0; i < cvarr_rgb.n_values; i++) {
	    G_debug(4, "cat = %d  %s = %s", cvarr_rgb.value[i].cat,
		    rgb_column, db_get_string(cvarr_rgb.value[i].val.s));
	}
    }

    if (width_column) {
	if (*width_column == '\0')
	    G_fatal_error(_("Line width column not specified"));

	db_CatValArray_init(&cvarr_width);

	nrec_width = db_select_CatValArray(driver, fi->table, fi->key,
					   width_column, NULL, &cvarr_width);

	G_debug(3, "nrec_width (%s) = %d", width_column, nrec_width);

	if (cvarr_width.ctype != DB_C_TYPE_INT &&
	    cvarr_width.ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Line width column (%s) not a number"),
			  width_column);

	if (nrec_width < 0)
	    G_fatal_error(_("Unable to select data (%s) from table"),
			  width_column);

	G_debug(2, "\n%d records selected from table", nrec_width);

	for (i = 0; i < cvarr_width.n_values; i++) {
	    G_debug(4, "cat = %d  %s = %d", cvarr_width.value[i].cat,
		    width_column,
		    (cvarr_width.ctype ==
		     DB_C_TYPE_INT ? cvarr_width.value[i].val.
		     i : (int)cvarr_width.value[i].val.d));
	}
    }

    if (size_column) {
	if (*size_column == '\0')
	    G_fatal_error(_("Symbol size column not specified"));
	
	db_CatValArray_init(&cvarr_size);

	nrec_size = db_select_CatValArray(driver, fi->table, fi->key,
					  size_column, NULL, &cvarr_size);
	
	G_debug(3, "nrec_size (%s) = %d", size_column, nrec_size);

	if (cvarr_size.ctype != DB_C_TYPE_INT &&
	    cvarr_size.ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Symbol size column (%s) is not numeric"),
			  size_column);

	if (nrec_size < 0)
	    G_fatal_error(_("Unable to select data (%s) from table"),
			  size_column);

	G_debug(2, " %d records selected from table", nrec_size);

	for (i = 0; i < cvarr_size.n_values; i++) {
	    G_debug(4, "(size) cat = %d  %s = %.2f", cvarr_size.value[i].cat,
		    size_column,
		    (cvarr_size.ctype ==
		     DB_C_TYPE_INT ? (double)cvarr_size.value[i].val.i
		     : cvarr_size.value[i].val.d));
	}
    }

    if (rot_column) {
	if (*rot_column == '\0')
	    G_fatal_error(_("Symbol rotation column not specified"));

	db_CatValArray_init(&cvarr_rot);

	nrec_rot = db_select_CatValArray(driver, fi->table, fi->key,
					 rot_column, NULL, &cvarr_rot);

	G_debug(3, "nrec_rot (%s) = %d", rot_column, nrec_rot);

	if (cvarr_rot.ctype != DB_C_TYPE_INT &&
	    cvarr_rot.ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Symbol rotation column (%s) is not numeric"),
			  rot_column);

	if (nrec_rot < 0)
	    G_fatal_error(_("Unable to select data (%s) from table"),
			  rot_column);

	G_debug(2, " %d records selected from table", nrec_rot);

	for (i = 0; i < cvarr_rot.n_values; i++) {
	    G_debug(4, "(rot) cat = %d  %s = %.2f", cvarr_rot.value[i].cat,
		    rot_column,
		    (cvarr_rot.ctype ==
		     DB_C_TYPE_INT ? (double)cvarr_rot.value[i].val.i
		     : cvarr_rot.value[i].val.d));
	}
    }

    if (open_db)
	db_close_database_shutdown_driver(driver);

    stat = 0;
    if (type & GV_AREA)
	stat += display_area(Map, Clist, window, 
			     bcolor, fcolor, chcat,
			     id_flag, table_colors_flag, cats_colors_flag,
			     rgb_column, default_width, width_column, width_scale,
			     z_color_flag, z_column,
			     &cvarr_rgb, &cvarr_width, nrec_width);
			     
    
    stat += display_lines(Map, type, Clist,
			  bcolor, fcolor, chcat,
			  icon, size, size_column, sqrt_flag, rot_column,
			  id_flag, table_colors_flag, cats_colors_flag,
			  rgb_column, default_width, width_column, width_scale,
			  z_color_flag, z_column,
			  &cvarr_rgb, &cvarr_width, nrec_width,
			  &cvarr_size, nrec_size, &cvarr_rot, nrec_rot);
    
    return stat;
}
