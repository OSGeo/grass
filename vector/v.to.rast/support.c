/*
*  Update a history file.  Some of the digit file information  is placed in
*  the hist file.
*  returns  0  -  successful creation of history file
*          -1  -  error
*/

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "local.h"


int update_hist (char *raster_name, char *vector_name,
    char *vector_mapset, long scale)
{
    struct History hist ;

    if(raster_name == NULL)
        return(-1) ;

    if (G_read_history (raster_name, G_mapset (), &hist) < 0)
	return -1;

    strcpy(hist.title, raster_name) ;

    /* store information from digit file into history */
    sprintf(hist.datsrc_1, "Vector Map: %s in mapset %s", vector_name, vector_mapset);
    sprintf(hist.datsrc_2, "Original scale from vector map: 1:%ld", scale) ;  /* 4.0 */

    /* store command line options */
    G_command_history(&hist);

    return (G_write_history(raster_name, &hist)) ;
}


int 
update_colors (char *raster_name)
{
    struct Range range;
    struct Colors colors;
    CELL min,max;

    G_read_range (raster_name, G_mapset(), &range);
    G_get_range_min_max (&range, &min, &max);
    G_make_rainbow_colors (&colors, min, max);
    G_write_colors (raster_name, G_mapset(), &colors);

    return 0;
}


int
update_fcolors (char *raster_name)
{
    struct FPRange range;
    struct Colors colors;
    DCELL min,max;

    G_read_fp_range(raster_name, G_mapset(), &range);
    G_get_fp_range_min_max(&range, &min, &max);
    G_make_rainbow_colors(&colors, (CELL)min, (CELL)max);
    G_write_colors(raster_name, G_mapset(), &colors);

    return 0;
}


int 
update_cats (char *raster_name)
{
    /* TODO: maybe attribute transfer from vector map? 
       Use G_set_raster_cat() somewhere*/
    
    struct Categories cats;

    G_strip(raster_name);
    G_init_cats ((CELL) 0, raster_name, &cats);
    G_write_cats (raster_name, &cats);

    return 0;
}

int update_dbcolors(char *rast_name, char *vector_map, int field, char *rgb_column, int is_fp, char *attr_column)
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

    struct My_color_rule {
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
    G_init_colors(&colors);

    /* open vector map and database driver */
    Vect_open_old (&Map, vector_map, G_find_vector2 (vector_map, ""));

    db_CatValArray_init ( &cvarr );
    if ((Fi = Vect_get_field (&Map, field)) == NULL)
	G_fatal_error (_("Database connection not defined for layer %d"),
		       field);

    if ((Driver = db_start_driver_open_database (Fi->driver, Fi->database)) == NULL)
        G_fatal_error (_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);

    /* get number of records in attr_column */
    if ((nrec = db_select_CatValArray (Driver, Fi->table, Fi->key, attr_column , NULL, &cvarr)) == -1)
        G_fatal_error (_("Unknown column <%s> in table <%s>"), attr_column, Fi->table);

    if (nrec < 0)
        G_fatal_error (_("No records selected from table <%s>"), Fi->table);

    G_debug (3, "nrec = %d", nrec );

    /* allocate space for color rules */
    my_color_rules = (struct My_color_rule *)G_malloc(sizeof(struct My_color_rule)*nrec);

    /* for each attribute */
    for ( i = 0; i < cvarr.n_values; i++ )
    {
        char colorstring[12];
        dbValue value;

        /* selecect color attribute and category */
        cat = cvarr.value[i].cat;
        if (db_select_value (Driver, Fi->table, Fi->key, cat, rgb_column, &value) < 0)
        {
            G_warning(_("No records selected"));
            continue;
        } 
        sprintf (colorstring, "%s", value.s.string);

        /* convert color string to three color integers */
        if (*colorstring != '\0') {
            G_debug (3, "element colorstring: %s", colorstring);
        
            if ( G_str_to_color(colorstring, &red, &grn, &blu) == 1) {
                G_debug (3, "cat %d r:%d g:%d b:%d", cat, red, grn, blu);
            } else { 
                G_warning(_("Error in color definition column (%s) "
                "with cat %d: colorstring [%s]"), rgb_column, cat, colorstring);
                G_warning(_("Color set to [200:200:200]"));
                red = grn = blu = 200;
            }
        } else {
            G_warning (_("Error in color definition column (%s), with cat %d"),
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
            G_debug(2,"val: %f rgb: %s", cvarr.value[i].val.d, colorstring);
        }
        else {
            my_color_rules[i].i = cvarr.value[i].val.i;
            G_debug(2,"val: %d rgb: %s", cvarr.value[i].val.i, colorstring);
        }
    } /* /for each value in database */

    /* close the database driver */
    db_close_database_shutdown_driver(Driver);

    /* set the color rules: for each rule*/
    for ( i = 0; i < colors_n_values -1; i++ ) {
        if (is_fp) { /* add floating point color rule */
            G_add_d_raster_color_rule (
                 &my_color_rules[i].d , my_color_rules[i].red, my_color_rules[i].green, my_color_rules[i].blue,
                 &my_color_rules[i+1].d , my_color_rules[i+1].red, my_color_rules[i+1].green, my_color_rules[i+1].blue,
                 &colors);
        } else { /* add CELL color rule */
             G_add_color_rule (
                 (CELL) my_color_rules[i].i , my_color_rules[i].red, my_color_rules[i].green, my_color_rules[i].blue,
                 (CELL) my_color_rules[i+1].i , my_color_rules[i+1].red, my_color_rules[i+1].green, my_color_rules[i+1].blue,
                 &colors);
        }
    }

    /* write the rules */
    G_write_colors(rast_name, G_mapset(), &colors);
    
    return 1;
}


/* add labels to raster cells */
int update_labels (char *rast_name, char *vector_map, int field,
            char *label_column, int use, int val, char *attr_column)
{
    int i;
    int fd;

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
    struct My_labels_rule {
        dbString label;
        double d;
        int i;
    } *my_labels_rules;

    /* init raster categories */
    G_init_cats ((CELL)0, "Categories", &rast_cats);

    if (!(fd = G_open_cell_old (rast_name, G_mapset ())))
        G_fatal_error (_("Unable to open raster map <%s>"), rast_name);

    switch (use)
    {
    case USE_ATTR:
    {
	G_set_raster_cats_title ("Labels", &rast_cats);
        int is_fp = G_raster_map_is_fp (rast_name, G_mapset ());

        /* open vector map and database driver */
        Vect_open_old (&Map, vector_map, G_find_vector2 (vector_map, ""));

        db_CatValArray_init (&cvarr);
        if (!(Fi = Vect_get_field (&Map, field)))
            G_fatal_error (_("Database connection not defined for layer %d"),
			   field);

        if (!(Driver = db_start_driver_open_database (Fi->driver, Fi->database)))
            G_fatal_error (_("Unable to open database <%s> by driver <%s>"), 
                        Fi->database, Fi->driver);

        /* get number of records in label_column */
        if ((nrec = db_select_CatValArray (Driver, Fi->table, Fi->key, attr_column, NULL, &cvarr)) == -1)
            G_fatal_error (_("Unknown column <%s> in table <%s>"), 
                        attr_column, Fi->table);

        if (nrec < 0)
            G_fatal_error (_("No records selected from table <%s>"), Fi->table);

        G_debug (3, "nrec = %d", nrec );

        my_labels_rules = (struct My_labels_rule *)G_malloc (sizeof(struct My_labels_rule)*nrec);

        /* get column type */
	if (!label_column) {
	    G_warning (_("Label column was not specified, no labels will be written"));
	    break;
	}
	else {
	    if ((col_type = db_column_Ctype (Driver, Fi->table, label_column)) == -1) {
		G_fatal_error (_("Column <%s> not found"), label_column);
	    }
	}
	    
	/* for each attribute */
        for (i = 0; i < cvarr.n_values; i++)
        {
            char tmp[64];
            dbValue value;
            int cat = cvarr.value[i].cat;

            if (db_select_value (Driver, Fi->table, Fi->key, cat, label_column, &value) < 0) {
                G_warning (_("No records selected"));
                continue;
            } 

            labels_n_values++;

            db_init_string (&my_labels_rules[i].label);

            /* switch the column type */
            switch (col_type)
            {
            case DB_C_TYPE_DOUBLE:
                sprintf (tmp, "%lf", db_get_value_double (&value));
                db_set_string (&my_labels_rules[i].label, tmp); 
                break;
            case DB_C_TYPE_INT:
                sprintf (tmp, "%d", db_get_value_int (&value));
                db_set_string (&my_labels_rules[i].label, tmp); 
                break;
            case DB_C_TYPE_STRING:
                db_set_string (&my_labels_rules[i].label, db_get_value_string (&value)); 
            break;
            default:
                G_warning (_("Column type (%s) not supported"),
			   db_sqltype_name(col_type));
            }

            /* add the raster category to label */
            if (is_fp)
                my_labels_rules[i].d = cvarr.value[i].val.d;
            else
                my_labels_rules[i].i = cvarr.value[i].val.i;
        } /* for each value in database */

        /* close the database driver */
        db_close_database_shutdown_driver(Driver);

        /* set the color rules: for each rule */
        if (is_fp) {  
            /* add label */
            for ( i = 0; i < labels_n_values -1; i++ )
                G_set_raster_cat (&my_labels_rules[i].d,
                                    &my_labels_rules[i+1].d, 
                                    db_get_string (&my_labels_rules[i].label),
                                    &rast_cats, DCELL_TYPE);
        } else {
            for ( i = 0; i < labels_n_values ; i++ )
                G_set_cat (my_labels_rules[i].i, 
                           db_get_string (&my_labels_rules[i].label),
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

        map_type = G_raster_map_type (rast_name, G_mapset ());
        G_set_raster_cats_title ("Values", &rast_cats);
        
        if (map_type == CELL_TYPE)
        {
            CELL min, max;

            G_read_range (rast_name, G_mapset (), &range);
            G_get_range_min_max (&range, &min, &max);

            sprintf (msg, "Value %d", val);
            G_set_raster_cat (&min, &max, msg, &rast_cats, map_type);
        } else {
            DCELL fmin, fmax;

            G_read_fp_range (rast_name, G_mapset (), &fprange);
            G_get_fp_range_min_max (&fprange, &fmin, &fmax);

            sprintf (msg, "Value %.4f", (double)val);
            G_set_raster_cat (&fmin, &fmax, msg, &rast_cats, map_type);
        }

    }
    break;
    case USE_CAT:
    {
        int row, rows;
        void *rowbuf;
        struct Cell_stats stats;
        CELL n;
        RASTER_MAP_TYPE map_type;
        char *mapset;
        long count;

        mapset = G_mapset ();

        if (!(fd = G_open_cell_old (rast_name, mapset)))
            G_fatal_error (_("Unable to open raster map <%s>"), rast_name);

        map_type = G_raster_map_type (rast_name, mapset);

        if (!(rowbuf = G_allocate_raster_buf (map_type)))
            G_fatal_error (_("Cannot allocate memory for row buffer"));

        G_init_cell_stats (&stats);
        G_set_raster_cats_title ("Categories", &rast_cats);

        rows = G_window_rows ();

        for (row = 0; row < rows; row++)
        {
            if (G_get_raster_row (fd, rowbuf, row, map_type) < 0)
                G_fatal_error (_("Unable to read raster map <%s> row %d"), rast_name, row);

            G_update_cell_stats (rowbuf, G_window_cols (), &stats);
        }

        G_rewind_cell_stats (&stats);

        while (G_next_cell_stat (&n, &count, &stats))
        {
            char msg[80];

            sprintf (msg, "Category %d", n);
            G_set_raster_cat (&n, &n, msg, &rast_cats, map_type);
        }

        G_free (rowbuf);
    }
    break;
    case USE_D:
    {
        DCELL fmin, fmax;
        RASTER_MAP_TYPE map_type;
        char *mapset;
        int i;
        char msg[64];

        mapset = G_mapset ();
        map_type = G_raster_map_type (rast_name, mapset);
        G_set_raster_cats_title ("Degrees", &rast_cats);

        for (i = 1; i <= 360; i++)
        {
            sprintf (msg, "%d degrees", i);

            if (i == 360) {
                fmin = 359.5;
                fmax = 360.0;
                G_set_raster_cat (&fmin, &fmax, msg, &rast_cats, map_type);
                fmin = 0.0;
                fmax = 0.5;
            } else {
                fmin = i - 0.5;
                fmax = i + 0.5;
            }

            G_set_raster_cat (&fmin, &fmax, msg, &rast_cats, map_type);
        }
    }
    break;
    case USE_Z:
    /* TODO or not TODO */
    break;
    default:
        G_fatal_error (_("Unknown use type: %d"), use);
    break;
    }

    G_close_cell (fd);
    if (G_write_cats (rast_name, &rast_cats) <= 0)
        G_warning (_("Unable to write categories for raster map <%s>"), rast_name);
    G_free_cats (&rast_cats);

    return 1;
}
