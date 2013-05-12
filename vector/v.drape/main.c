
/**********************************************************
 *
 * MODULE:       v.drape
 * 
 * AUTHOR(S):    Radim Blazek, Dylan Beaudette
 *               Maris Nartiss - WHERE support and raster NULL support
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Convert 2D vector to 3D vector by sampling of elevation raster.
 *               
 * COPYRIGHT:    (C) 2005-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 * 
 **********************************************************/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* Samples raster map */
int sample_raster(const int ltype, int fdrast, struct Cell_head window,
		  struct line_pnts *Points, const INTERP_TYPE method,
		  const double scale, const struct Option *null_opt,
		  const double null_val)
{
    double estimated_elevation;
    int j;

    /* adjust flow based on specific type of line */
    switch (ltype) {
	/* points (at least 1 vertex) */
    case GV_POINT:
    case GV_CENTROID:
    case GV_KERNEL:
	/* sample raster at this point, and update the z-coordinate
	 * (note that input vector should not be 3D!)
	 */
	estimated_elevation = scale * Rast_get_sample(
	    fdrast, &window, NULL, Points->y[0], Points->x[0], 0, method);
	/* Elevation value has to be meaningfull */
	if (Rast_is_d_null_value(&estimated_elevation)) {
	    if (null_opt->answer) {
		estimated_elevation = null_val;
	    }
	    else {
		return 0;
	    }
	}
	/* update the elevation value for each data point */
	Points->z[0] = estimated_elevation;
	break;
	/* standard lines (at least 2 vertexes) */
    case GV_LINE:
    case GV_BOUNDARY:
	if (Points->n_points < 2)
	    break;		/* At least 2 points */

	/* loop through each point in a line */
	for (j = 0; j < Points->n_points; j++) {
	    /* sample raster at this point, and update the z-coordinate (note that input vector should not be 3D!) */
	    estimated_elevation = scale * Rast_get_sample(
		fdrast, &window, NULL, Points->y[j], Points->x[j], 0, method);

	    if (Rast_is_d_null_value(&estimated_elevation)) {
		if (null_opt->answer) {
		    estimated_elevation = null_val;
		}
		else {
		    return 0;
		}
	    }
	    /* update the elevation value for each data point */
	    Points->z[j] = estimated_elevation;
	}			/* end looping through point in a line */
	break;

	/* lines with at least 3 vertexes */
    case GV_FACE:
	if (Points->n_points < 3)
	    break;		/* At least 3 points */

	/* loop through each point in a line */
	for (j = 0; j < Points->n_points; j++) {
	    /* sample raster at this point, and update the z-coordinate (note that input vector should not be 3D!) */
	    estimated_elevation = scale * Rast_get_sample(
		fdrast, &window, NULL, Points->y[j], Points->x[j], 0, method);

	    if (Rast_is_d_null_value(&estimated_elevation)) {
		if (null_opt->answer) {
		    estimated_elevation = null_val;
		}
		else {
		    return 0;
		}
	    }
	    /* update the elevation value for each data point */
	    Points->z[j] = estimated_elevation;
	}
	break;
    }				/* end line type switch */
    return 1;
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *out_opt, *type_opt, *rast_opt, *method_opt,
	*scale_opt, *where_opt, *layer_opt, *null_opt;
    
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct field_info *Fi;

    int line, nlines, otype, ltype, ncats =
	0, layer, i, c, *cats, field_index, id, out_num = 0, *new_cats;

    double scale, null_val;
    INTERP_TYPE method = UNKNOWN;
    int fdrast;			/* file descriptor for raster map is int */
    struct Cell_head window;
    struct bound_box map_box;
    
    dbDriver *driver;
    dbHandle handle;
    dbTable *table;
    dbString table_name, dbsql, valstr;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("3D"));

    module->description =
	_("Converts vector map to 3D by sampling of elevation raster map.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    layer_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    type_opt = G_define_standard_option(G_OPT_V3_TYPE);
    
    /* raster sampling */
    rast_opt = G_define_standard_option(G_OPT_R_MAP);
    rast_opt->key = "rast";
    rast_opt->description = _("Elevation raster map for height extraction");
    
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    method_opt = G_define_standard_option(G_OPT_R_INTERP_TYPE);
    method_opt->answer = "nearest";
    method_opt->description = _("Sampling method");
    
    scale_opt = G_define_option();
    scale_opt->key = "scale";
    scale_opt->type = TYPE_DOUBLE;
    scale_opt->description = _("Scale sampled raster values");
    scale_opt->answer = "1.0";

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);

    null_opt = G_define_option();
    null_opt->key = "null_value";
    null_opt->type = TYPE_DOUBLE;
    null_opt->label = _("Vector Z value for unknown height");
    null_opt->description =
	_("Will set Z to this value, if value from raster map can not be read");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    if (null_opt->answer)
	null_val = atof(null_opt->answer);

    /* which interpolation method should we use */
    method = Rast_option_to_interp_type(method_opt);
    
    /* setup the region */
    G_get_window(&window);

    /* used to scale sampled raster values */
    scale = atof(scale_opt->answer);

    /* Check output type */
    otype = Vect_option_to_types(type_opt);

    /* open the elev raster, and check for error condition */
    fdrast = Rast_open_old(rast_opt->answer, "");
    
    /* check input/output vector maps */
    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 G_FATAL_EXIT);
    
    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", layer_opt->answer);
    layer = Vect_get_field_number(&In, layer_opt->answer);

    if (where_opt->answer) {
	/* Let's get vector layers db connections information */
	Fi = Vect_get_field(&In, layer);
	if (!Fi) {
	    Vect_close(&In);
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  layer);
	}
	G_debug(1,
		"Field number:%d; Name:<%s>; Driver:<%s>; Database:<%s>; Table:<%s>; Key:<%s>",
		Fi->number, Fi->name, Fi->driver, Fi->database, Fi->table,
		Fi->key);
	/* Prepeare strings for use in db_* calls */
	db_init_string(&dbsql);
	db_init_string(&valstr);
	db_init_string(&table_name);
	db_init_handle(&handle);

	/* Prepearing database for use */
	driver = db_start_driver(Fi->driver);
	if (driver == NULL) {
	    Vect_close(&In);
	    G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
	}
	db_set_handle(&handle, Fi->database, NULL);
	if (db_open_database(driver, &handle) != DB_OK) {
	    Vect_close(&In);
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, driver);
	}
	db_set_string(&table_name, Fi->table);
	if (db_describe_table(driver, &table_name, &table) != DB_OK) {
	    Vect_close(&In);
	    G_fatal_error(_("Unable to describe table <%s>"), Fi->table);
	}
	ncats =
	    db_select_int(driver, Fi->table, Fi->key, where_opt->answer,
			  &cats);
	if (ncats < 1)
	    G_fatal_error(_("No features match Your query"));
	G_debug(3, "Number of features matching query: %d", ncats);
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* line types */
    if ((otype &
	 (GV_POINTS | GV_LINES | GV_BOUNDARY | GV_CENTROID | GV_FACE |
	  GV_KERNEL))) {

	/* process all lines matching WHERE statement */
	if (where_opt->answer) {
	    field_index = Vect_cidx_get_field_index(&In, layer);
	    for (i = 0; i < ncats; i++) {
		G_percent(i, ncats, 2);
                
		c = Vect_cidx_find_next(&In, field_index, cats[i],
					otype, 0, &ltype, &id);
		while (c >= 0) {
		    c++;
		    if (ltype & otype) {
			if (Vect_read_line(&In, Points, Cats, id) == ltype)
			    if (sample_raster
				(ltype, fdrast, window, Points, method,
				 scale, null_opt, null_val)) {
				
                                /* Open new file only if there is something to write in */
				if (out_num < 1) {
				    if (0 >
					Vect_open_new(&Out, out_opt->answer,
						      WITH_Z)) {
					Vect_close(&In);
					G_fatal_error(_("Unable to create vector map <%s>"),
						      out_opt->answer);
				    }
				    Vect_copy_head_data(&In, &Out);
				    Vect_hist_copy(&In, &Out);
				    Vect_hist_command(&Out);
				}
                                
				Vect_write_line(&Out, ltype, Points, Cats);
				out_num++;
			    }
		    }
		    c = Vect_cidx_find_next(&In, field_index, cats[i],
					    otype, c, &ltype, &id);
		}
	    }
	}
	else {
	    /* loop through each line in the dataset */
	    nlines = Vect_get_num_lines(&In);

	    for (line = 1; line <= nlines; line++) {

		/* progress feedback */
		G_percent(line, nlines, 2);

		/* get the line type */
		ltype = Vect_read_line(&In, Points, Cats, line);
		if (layer != -1 && !Vect_cat_get(Cats, layer, NULL))
		    continue;
		
		/* write the new line file, with the updated Points struct */
		if (sample_raster
		    (ltype, fdrast, window, Points, method, scale, null_opt,
		     null_val)) {
		    
                    /* Open new file only if there is something to write in */
		    if (out_num < 1) {
			if (0 > Vect_open_new(&Out, out_opt->answer, WITH_Z)) {
			    Vect_close(&In);
			    G_fatal_error(_("Unable to create vector map <%s>"),
					  out_opt->answer);
			}
			Vect_copy_head_data(&In, &Out);
			Vect_hist_copy(&In, &Out);
			Vect_hist_command(&Out);
		    }
                    
		    Vect_write_line(&Out, ltype, Points, Cats);
		    out_num++;
		}
	    }			/* end looping thru lines */
	}

    }				/* end working on type=lines */

    /* close elevation raster: */
    Rast_close(fdrast);
    
    /* build topology for output vector */
    if (out_num > 0) {
	Vect_build(&Out);
        
        /* Now let's move attribute data from all old map layers to new map */
        for (i = 0; i < Out.plus.n_cidx; i++) {
            new_cats = (int *)G_malloc(Out.plus.cidx[i].n_cats * sizeof(int));
            if (!new_cats) {
                G_warning(_("Due to error attribute data to new map are not transferred"));
                break;
            }
            /* Vect_copy_table_by_cats does not accept Map.plus.cidx[].cat array as input.
            	Thus we construct new array of cats. */
            for (c = 0; c < Out.plus.cidx[i].n_cats; c++) {
                new_cats[c] = Out.plus.cidx[i].cat[c][0];
            }
            if (0 > Vect_copy_table_by_cats(&In, &Out, In.plus.cidx[i].field, Out.plus.cidx[i].field,
                    NULL, otype, new_cats, Out.plus.cidx[i].n_cats))
                    G_warning(_("Due to error attribute data to new map are not transferred"));
        }

	Vect_get_map_box(&Out, &map_box);

	/* close output vector */
	Vect_close(&Out);
    }
    else {
        /* close input vector */
        Vect_close(&In);
	G_warning(_("No features drapped. Check your computational region and input vector map."));
	exit(EXIT_SUCCESS);
    }

    /* close input vector */
    Vect_close(&In);

    G_done_msg("T: %f B: %f.", map_box.T, map_box.B);
    
    exit(EXIT_SUCCESS);
}
