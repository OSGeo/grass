/*
 *  Update a history file.  Some of the digit file information  is placed in
 *  the hist file.
 *  returns  0  -  successful creation of history file
 *          -1  -  error
 */

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local.h"


int update_hist(const char *raster_name, const char *vector_name, long scale)
{
    struct History hist;

    if (raster_name == NULL)
	return (-1);

    Rast_short_history(raster_name, "raster", &hist);

    /* store information from digit file into history */
    Rast_format_history(&hist, HIST_DATSRC_1,
			"Vector Map: %s", vector_name);
    Rast_format_history(&hist, HIST_DATSRC_2,
			"Original scale from vector map: 1:%ld", scale);	/* 4.0 */

    /* store command line options */
    Rast_command_history(&hist);

    Rast_write_history(raster_name, &hist);

    return 0;
}


int update_colors(const char *raster_name)
{
    struct Range range;
    struct Colors colors;
    CELL min, max;

    Rast_read_range(raster_name, G_mapset(), &range);
    Rast_get_range_min_max(&range, &min, &max);
    Rast_make_rainbow_colors(&colors, min, max);
    Rast_write_colors(raster_name, G_mapset(), &colors);

    return 0;
}


int update_fcolors(const char *raster_name)
{
    struct FPRange range;
    struct Colors colors;
    DCELL min, max;

    Rast_read_fp_range(raster_name, G_mapset(), &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    Rast_make_rainbow_colors(&colors, (CELL) min, (CELL) max);
    Rast_write_colors(raster_name, G_mapset(), &colors);

    return 0;
}


int update_cats(const char *raster_name)
{
    /* TODO: maybe attribute transfer from vector map? 
       Use Rast_set_cat() somewhere */

    struct Categories cats;

    Rast_init_cats(raster_name, &cats);
    Rast_write_cats(raster_name, &cats);

    return 0;
}

int update_dbcolors(const char *rast_name, const char *vector_map, int field,
		    const char *rgb_column, int is_fp, const char *attr_column)
{
    int i;

    /* Map */
    struct Map_info Map;

    /* Attributes */
    int nrec;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;

    /* colors */
    int cat;
    struct Colors colors;

    struct My_color_rule
    {
	int red;
	int green;
	int blue;
	double d;
	int i;
    } *my_color_rules;

    int colors_n_values = 0;
    int red;
    int grn;
    int blu;

    /* init colors structure */
    Rast_init_colors(&colors);

    /* open vector map and database driver */
    Vect_open_old(&Map, vector_map, "");

    db_CatValArray_init(&cvarr);
    if ((Fi = Vect_get_field(&Map, field)) == NULL)
	G_fatal_error(_("Database connection not defined for layer %d"),
		      field);

    if ((Driver =
	 db_start_driver_open_database(Fi->driver, Fi->database)) == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    if (!attr_column)
	attr_column = Fi->key;
    
    /* get number of records in attr_column */
    if ((nrec =
	 db_select_CatValArray(Driver, Fi->table, Fi->key, attr_column, NULL,
			       &cvarr)) == -1)
	G_fatal_error(_("Unknown column <%s> in table <%s>"), attr_column,
		      Fi->table);

    if (nrec < 0)
	G_fatal_error(_("No records selected from table <%s>"), Fi->table);

    G_debug(3, "nrec = %d", nrec);

    /* allocate space for color rules */
    my_color_rules =
	(struct My_color_rule *)G_malloc(sizeof(struct My_color_rule) * nrec);

    /* for each attribute */
    for (i = 0; i < cvarr.n_values; i++) {
	char colorstring[12];
	dbValue value;

	/* selecect color attribute and category */
	cat = cvarr.value[i].cat;
	if (db_select_value
	    (Driver, Fi->table, Fi->key, cat, rgb_column, &value) < 0) {
	    G_warning(_("No records selected"));
	    continue;
	}
	sprintf(colorstring, "%s", value.s.string);

	/* convert color string to three color integers */
	if (*colorstring != '\0') {
	    G_debug(3, "element colorstring: %s", colorstring);

	    if (G_str_to_color(colorstring, &red, &grn, &blu) == 1) {
		G_debug(3, "cat %d r:%d g:%d b:%d", cat, red, grn, blu);
	    }
	    else {
		G_warning(_("Error in color definition column (%s) "
			    "with cat %d: colorstring [%s]"), rgb_column, cat,
			  colorstring);
		G_warning(_("Color set to [200:200:200]"));
		red = grn = blu = 200;
	    }
	}
	else {
	    G_warning(_("Error in color definition column (%s), with cat %d"),
		      rgb_column, cat);
	}

	/* append color rules to my_color_rules array, they will be set
	 * later all togheter */
	colors_n_values++;
	my_color_rules[i].red = red;
	my_color_rules[i].green = grn;
	my_color_rules[i].blue = blu;
	if (is_fp) {
	    my_color_rules[i].d = cvarr.value[i].val.d;
	    G_debug(2, "val: %f rgb: %s", cvarr.value[i].val.d, colorstring);
	}
	else {
	    my_color_rules[i].i = cvarr.value[i].val.i;
	    G_debug(2, "val: %d rgb: %s", cvarr.value[i].val.i, colorstring);
	}
    }				/* /for each value in database */

    /* close the database driver */
    db_close_database_shutdown_driver(Driver);

    /* set the color rules: for each rule */
    for (i = 0; i < colors_n_values - 1; i++) {
	if (is_fp) {		/* add floating point color rule */
	    Rast_add_d_color_rule(&my_color_rules[i].d,
				      my_color_rules[i].red,
				      my_color_rules[i].green,
				      my_color_rules[i].blue,
				      &my_color_rules[i + 1].d,
				      my_color_rules[i + 1].red,
				      my_color_rules[i + 1].green,
				      my_color_rules[i + 1].blue, &colors);
	}
	else {			/* add CELL color rule */
	    Rast_add_c_color_rule(&my_color_rules[i].i,
				  my_color_rules[i].red, my_color_rules[i].green,
				  my_color_rules[i].blue,
				  &my_color_rules[i + 1].i,
				  my_color_rules[i + 1].red,
				  my_color_rules[i + 1].green,
				  my_color_rules[i + 1].blue, &colors);
	}
    }

    /* write the rules */
    Rast_write_colors(rast_name, G_mapset(), &colors);

    return 1;
}


/* add labels to raster cells */
int update_labels(const char *rast_name, const char *vector_map, int field,
		  const char *label_column, int use, int val,
		  const char *attr_column)
{
    int i;

    /* Map */
    struct Map_info Map;

    /* Attributes */
    int nrec;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;
    int col_type;

    /* labels */
    struct Categories rast_cats;
    int labels_n_values = 0;
    struct My_labels_rule
    {
	dbString label;
	double d;
	int i;
    } *my_labels_rules;

    /* init raster categories */
    Rast_init_cats("Categories", &rast_cats);

    switch (use) {
    case USE_ATTR:
	{
	    int is_fp = Rast_map_is_fp(rast_name, G_mapset());

	    if (!label_column) {
		G_verbose_message(_("Label column was not specified, no labels will be written"));
		break;
	    }

	    Rast_set_cats_title("Rasterized vector map from labels", &rast_cats);

	    /* open vector map and database driver */
	    Vect_set_open_level(1);
	    Vect_open_old(&Map, vector_map, G_find_vector2(vector_map, ""));

	    db_CatValArray_init(&cvarr);
	    if (!(Fi = Vect_get_field(&Map, field)))
		G_fatal_error(_("Database connection not defined for layer %d"),
			      field);

	    Vect_close(&Map);

	    if (!
		(Driver =
		 db_start_driver_open_database(Fi->driver, Fi->database)))
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Fi->database, Fi->driver);

	    /* get number of records in label_column */
	    if ((nrec =
		 db_select_CatValArray(Driver, Fi->table, Fi->key,
				       attr_column, NULL, &cvarr)) == -1)
		G_fatal_error(_("Unknown column <%s> in table <%s>"),
			      attr_column, Fi->table);

	    if (nrec < 0)
		G_fatal_error(_("No records selected from table <%s>"),
			      Fi->table);

	    G_debug(3, "nrec = %d", nrec);

	    my_labels_rules =
		(struct My_labels_rule *)
		G_malloc(sizeof(struct My_labels_rule) * nrec);

	    /* get column type */
	    if ((col_type =
		 db_column_Ctype(Driver, Fi->table,
				 label_column)) == -1) {
		G_fatal_error(_("Column <%s> not found"), label_column);
	    }

	    /* for each attribute */
	    for (i = 0; i < cvarr.n_values; i++) {
		char tmp[64];
		dbValue value;
		int cat = cvarr.value[i].cat;

		if (db_select_value
		    (Driver, Fi->table, Fi->key, cat, label_column,
		     &value) < 0) {
		    G_warning(_("No records selected"));
		    continue;
		}

		labels_n_values++;

		db_init_string(&my_labels_rules[i].label);

		/* switch the column type */
		switch (col_type) {
		case DB_C_TYPE_DOUBLE:
		    sprintf(tmp, "%lf", db_get_value_double(&value));
		    db_set_string(&my_labels_rules[i].label, tmp);
		    break;
		case DB_C_TYPE_INT:
		    sprintf(tmp, "%d", db_get_value_int(&value));
		    db_set_string(&my_labels_rules[i].label, tmp);
		    break;
		case DB_C_TYPE_STRING:
		    db_set_string(&my_labels_rules[i].label,
				  db_get_value_string(&value));
		    break;
		default:
		    G_warning(_("Column type (%s) not supported"),
			      db_sqltype_name(col_type));
		}

		/* add the raster category to label */
		if (is_fp)
		    my_labels_rules[i].d = cvarr.value[i].val.d;
		else
		    my_labels_rules[i].i = cvarr.value[i].val.i;
	    }			/* for each value in database */

	    /* close the database driver */
	    db_close_database_shutdown_driver(Driver);

	    /* set the color rules: for each rule */
	    if (is_fp) {
		/* add label */
		for (i = 0; i < labels_n_values - 1; i++)
		    Rast_set_cat(&my_labels_rules[i].d,
				     &my_labels_rules[i + 1].d,
				     db_get_string(&my_labels_rules[i].label),
				     &rast_cats, DCELL_TYPE);
	    }
	    else {
		for (i = 0; i < labels_n_values; i++)
		  Rast_set_c_cat(&(my_labels_rules[i].i),
				 &(my_labels_rules[i].i),
				 db_get_string(&my_labels_rules[i].label),
				 &rast_cats);
	    }
	}
	break;
    case USE_VAL:
	{
	    char msg[64];
	    RASTER_MAP_TYPE map_type;
	    struct FPRange fprange;
	    struct Range range;

	    map_type = Rast_map_type(rast_name, G_mapset());
	    Rast_set_cats_title("Rasterized vector map from values", &rast_cats);

	    if (map_type == CELL_TYPE) {
		CELL min, max;

		Rast_read_range(rast_name, G_mapset(), &range);
		Rast_get_range_min_max(&range, &min, &max);

		sprintf(msg, "Value %d", val);
		Rast_set_cat(&min, &max, msg, &rast_cats, map_type);
	    }
	    else {
		DCELL fmin, fmax;

		Rast_read_fp_range(rast_name, G_mapset(), &fprange);
		Rast_get_fp_range_min_max(&fprange, &fmin, &fmax);

		sprintf(msg, "Value %.4f", (double)val);
		Rast_set_cat(&fmin, &fmax, msg, &rast_cats, map_type);
	    }

	}
	break;
    case USE_CAT:
	{
	    int row, rows, fd;
	    void *rowbuf;
	    struct Cell_stats stats;
	    CELL n;
	    RASTER_MAP_TYPE map_type;
	    long count;

	    map_type = Rast_map_type(rast_name, G_mapset());

	    if (label_column) {

		Rast_set_cats_title("Rasterized vector map from labels", &rast_cats);

		/* open vector map and database driver */
		Vect_set_open_level(1);
		Vect_open_old(&Map, vector_map, G_find_vector2(vector_map, ""));

		db_CatValArray_init(&cvarr);
		if (!(Fi = Vect_get_field(&Map, field)))
		    G_fatal_error(_("Database connection not defined for layer %d"),
				  field);

		Vect_close(&Map);

		if (!
		    (Driver =
		     db_start_driver_open_database(Fi->driver, Fi->database)))
		    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
				  Fi->database, Fi->driver);

		/* get number of records in label_column */
		if ((nrec =
		     db_select_CatValArray(Driver, Fi->table, Fi->key,
					   label_column, NULL, &cvarr)) == -1)
		    G_fatal_error(_("Unknown column <%s> in table <%s>"),
				  label_column, Fi->table);

		if (nrec < 0)
		    G_fatal_error(_("No records selected from table <%s>"),
				  Fi->table);

		G_debug(3, "nrec = %d", nrec);

		my_labels_rules =
		    (struct My_labels_rule *)
		    G_malloc(sizeof(struct My_labels_rule) * nrec);

		/* get column type */
		if ((col_type =
		     db_column_Ctype(Driver, Fi->table,
				     label_column)) == -1) {
		    G_fatal_error(_("Column <%s> not found"), label_column);
		}

		/* close the database driver */
		db_close_database_shutdown_driver(Driver);

		/* for each attribute */
		for (i = 0; i < cvarr.n_values; i++) {
		    char tmp[2000];
		    int cat = cvarr.value[i].cat;
		    labels_n_values++;

		    db_init_string(&my_labels_rules[i].label);

		    /* switch the column type */
		    switch (col_type) {
		    case DB_C_TYPE_DOUBLE:
			sprintf(tmp, "%lf", cvarr.value[i].val.d);
			db_set_string(&my_labels_rules[i].label, tmp);
			break;
		    case DB_C_TYPE_INT:
			sprintf(tmp, "%d", cvarr.value[i].val.i);
			db_set_string(&my_labels_rules[i].label, tmp);
			break;
		    case DB_C_TYPE_STRING:
			db_set_string(&my_labels_rules[i].label,
				      db_get_string(cvarr.value[i].val.s));
			break;
		    default:
			G_warning(_("Column type (%s) not supported"),
				  db_sqltype_name(col_type));
		    }

		    /* add the raster category to label */
		    my_labels_rules[i].i = cat;

		    Rast_set_cat(&(my_labels_rules[i].i),
				 &(my_labels_rules[i].i),
				 db_get_string(&my_labels_rules[i].label),
				 &rast_cats, map_type);
		}			/* for each value in database */
	    }
	    else  {
		fd = Rast_open_old(rast_name, G_mapset());

		rowbuf = Rast_allocate_buf(map_type);

		Rast_init_cell_stats(&stats);
		Rast_set_cats_title("Rasterized vector map from categories", &rast_cats);

		rows = Rast_window_rows();

		for (row = 0; row < rows; row++) {
		    Rast_get_row(fd, rowbuf, row, map_type);
		    Rast_update_cell_stats(rowbuf, Rast_window_cols(), &stats);
		}

		Rast_rewind_cell_stats(&stats);

		while (Rast_next_cell_stat(&n, &count, &stats)) {
		    char msg[80];

		    sprintf(msg, "Category %d", n);
		    Rast_set_cat(&n, &n, msg, &rast_cats, map_type);
		}

		Rast_close(fd);
		G_free(rowbuf);
	    }
	}
	break;
    case USE_D:
	{
	    DCELL fmin, fmax;
	    RASTER_MAP_TYPE map_type;
	    int i;
	    char msg[64];

	    map_type = Rast_map_type(rast_name, G_mapset());
	    Rast_set_cats_title("Rasterized vector map from line direction", &rast_cats);
	    Rast_write_units(rast_name, "degrees CCW from +x");

	    for (i = 1; i <= 360; i++) {
		sprintf(msg, "%d degrees", i);

		if (i == 360) {
		    fmin = 359.5;
		    fmax = 360.0;
		    Rast_set_cat(&fmin, &fmax, msg, &rast_cats, map_type);
		    fmin = 0.0;
		    fmax = 0.5;
		}
		else {
		    fmin = i - 0.5;
		    fmax = i + 0.5;
		}

		Rast_set_cat(&fmin, &fmax, msg, &rast_cats, map_type);
	    }
	}
	break;
    case USE_Z:
	/* TODO or not TODO */
	break;
    default:
	G_fatal_error(_("Unknown use type: %d"), use);
	break;
    }

    Rast_write_cats(rast_name, &rast_cats);
    Rast_free_cats(&rast_cats);

    return 1;
}
