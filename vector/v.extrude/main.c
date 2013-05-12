
/****************************************************************
 *
 * MODULE:     v.extrude
 *
 * AUTHOR(S):  Jachym Cepicky <jachym.cepicky gmail.com>
 *             Support for points & OGR support (08/2007, 2009), new
 *             parameters cats, where, method (2013) added by Martin
 *             Landa <landa.martin gmail.com>
 *
 * PURPOSE:    "Extrudes" flat polygons and lines to 3D with defined height
 *             Useful for creating buildings for displaying with NVIZ
 *
 *             Based on v.example by Radim Blazek
 *             Inspired by d.vect and v.drape
 *             Coding help and code cleaning by Markus Neteler
 *
 * COPYRIGHT:  (C) 2005-2010,2013 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2). Read the file COPYING that comes
 *             with GRASS for details.
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
        struct Option *input, *output, *zshift, *height, *elevation, *hcolumn,
            *type, *field, *cats, *where, *interp;
    } opt;
    struct {
        struct Flag *trace;
    } flag;
    
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct bound_box map_box;

    struct cat_list *cat_list;
    
    struct Cell_head window;
    
    int field;
    int only_type, cat;
    int fdrast, interp_method, trace;
    double objheight, objheight_default, voffset;
    
    struct field_info *Fi;
    dbDriver *driver = NULL;
    
    char *comment;
    
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("3D"));
    module->description =
	_("Extrudes flat vector features to 3D with defined height.");

    flag.trace = G_define_flag();
    flag.trace->key = 't';
    flag.trace->description = _("Trace elevation");
    flag.trace->guisection = _("Elevation");

    opt.input = G_define_standard_option(G_OPT_V_INPUT);

    opt.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt.field->guisection = _("Selection");

    opt.cats = G_define_standard_option(G_OPT_V_CATS);
    opt.cats->guisection = _("Selection");
    
    opt.where = G_define_standard_option(G_OPT_DB_WHERE);
    opt.where->guisection = _("Selection");

    opt.type = G_define_standard_option(G_OPT_V_TYPE);
    opt.type->answer = "point,line,area";
    opt.type->options = "point,line,area";
    opt.type->guisection = _("Selection");

    opt.output = G_define_standard_option(G_OPT_V_OUTPUT);

    opt.zshift = G_define_option();
    opt.zshift->key = "zshift";
    opt.zshift->description = _("Shifting value for z coordinates");
    opt.zshift->type = TYPE_DOUBLE;
    opt.zshift->required = NO;
    opt.zshift->answer = "0";
    opt.zshift->guisection = _("Height");

    opt.height = G_define_option();
    opt.height->key = "height";
    opt.height->type = TYPE_DOUBLE;
    opt.height->required = NO;
    opt.height->multiple = NO;
    opt.height->description = _("Fixed height for 3D vector features");
    opt.height->guisection = _("Height");

    opt.hcolumn = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.hcolumn->key = "hcolumn";
    opt.hcolumn->multiple = NO;
    opt.hcolumn->description = _("Name of attribute column with feature height");
    opt.hcolumn->guisection = _("Height");
 
    /* raster sampling */
    opt.elevation = G_define_standard_option(G_OPT_R_ELEV);
    opt.elevation->required = NO;
    opt.elevation->description = _("Elevation raster map for height extraction");
    opt.elevation->guisection = _("Elevation");

    opt.interp = G_define_standard_option(G_OPT_R_INTERP_TYPE);
    opt.interp->answer = "nearest";
    opt.interp->guisection = _("Elevation");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!opt.height->answer && !opt.hcolumn->answer) {
	G_fatal_error(_("One of '%s' or '%s' parameters must be set"),
		      opt.height->key, opt.hcolumn->key);
    }

    sscanf(opt.zshift->answer, "%lf", &voffset);
    G_debug(1, "voffset = %f", voffset);
    
    if (opt.height->answer)
	sscanf(opt.height->answer, "%lf", &objheight);
    else
	objheight = 0.;
    G_debug(1, "objheight = %f", objheight);
    objheight_default = objheight;

    only_type = Vect_option_to_types(opt.type);
    interp_method = Rast_option_to_interp_type(opt.interp);

    trace = flag.trace->answer ? TRUE : FALSE;
    
    /* set input vector map name and mapset */
    Vect_check_input_output_name(opt.input->answer, opt.output->answer, G_FATAL_EXIT);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_set_open_level(2); /* topology required for input */
    /* opening input vector map */
    Vect_open_old2(&In, opt.input->answer, "", opt.field->answer);
    Vect_set_error_handler_io(&In, &Out);

    /* creating output vector map */
    Vect_open_new(&Out, opt.output->answer, WITH_Z);

    field = Vect_get_field_number(&In, opt.field->answer);

    if ((opt.hcolumn->answer || opt.cats->answer || opt.where->answer) && field == -1) {
        G_warning(_("Invalid layer number (%d). "
                    "Parameter '%s', '%s' or '%s' specified, assuming layer '1'."),
                  field, opt.hcolumn->key, opt.cats->key, opt.where->key);
        field = 1;
    }

    /* set constraint for cats or where */
    cat_list = NULL;
    if (field > 0)
	cat_list = Vect_cats_set_constraint(&In, field, opt.where->answer,
                                            opt.cats->answer);
    
    
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* opening database connection, if required */
    if (opt.hcolumn->answer) {
        int ctype;
        dbColumn *column;
        
	if ((Fi = Vect_get_field(&In, field)) == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  field);

	if ((driver =
	     db_start_driver_open_database(Fi->driver, Fi->database)) == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
        db_set_error_handler_driver(driver);
        
	if (db_get_column(driver, Fi->table, opt.hcolumn->answer, &column) != DB_OK)
	    G_fatal_error(_("Column <%s> does not exist"),
			  opt.hcolumn->answer);
	else
	    db_free_column(column);

	ctype = db_column_Ctype(driver, Fi->table, opt.hcolumn->answer);

	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_STRING &&
	    ctype != DB_C_TYPE_DOUBLE) {
	    G_fatal_error(_("Column <%s>: invalid data type"),
			  opt.hcolumn->answer);
	}
    }

    /* do we work with elevation raster? */
    fdrast = -1;
    if (opt.elevation->answer) {
	/* raster setup */
	G_get_window(&window);

	/* open the elev raster, and check for error condition */
	fdrast = Rast_open_old(opt.elevation->answer, "");
    }

    /* if area */
    if (only_type & GV_AREA) {
        int area, nareas, centroid;
        
        nareas = Vect_get_num_areas(&In);
	G_debug(2, "n_areas = %d", nareas);
	if (nareas > 0)
	    G_message(_("Extruding areas..."));
	for (area = 1; area <= nareas; area++) {
	    G_debug(3, "area = %d", area);
	    G_percent(area, nareas, 2);
            
	    if (!Vect_area_alive(&In, area))
		continue;
            
	    centroid = Vect_get_area_centroid(&In, area);
	    if (!centroid) {
		G_warning(_("Skipping area %d without centroid"), area);
		continue;
	    }

	    Vect_read_line(&In, NULL, Cats, centroid);
	    if (field > 0 && !Vect_cats_in_constraint(Cats, field, cat_list))
		continue;
            
	    /* height attribute */
	    if (opt.hcolumn->answer) {
		cat = Vect_get_area_cat(&In, area, field);
                if (cat == -1) {
                    G_warning(_("No category defined for area %d. Using default fixed height %f."),
                              area, objheight_default);
                    objheight = objheight_default;
                }
                if (get_height(Fi, opt.hcolumn->answer,
                               driver, cat, &objheight) != 0) {
                    G_warning(_("Unable to fetch height from DB for area %d. Using default fixed height %f."),
                              area, objheight_default);
                    objheight = objheight_default;
                }
	    } /* if opt.hcolumn->answer */

	    Vect_get_area_points(&In, area, Points);

	    G_debug(3, "area: %d height: %f", area, objheight);

	    extrude(&In, &Out, Cats, Points,
		    fdrast, trace, interp_method,
                    objheight, voffset, &window, GV_AREA,
		    centroid);
	} /* foreach area */

    }

    if (only_type > 0) {
        int line, nlines;
        int type;
        
	G_debug(1, "other than areas");
	/* loop through each line in the dataset */
        nlines = Vect_get_num_lines(&In);
	G_message(_("Extruding features..."));
	for (line = 1; line <= nlines; line++) {
	    /* progress feedback */
	    G_percent(line, nlines, 2);

	    if (!Vect_line_alive(&In, line))
		continue;

	    /* read line */
	    type = Vect_read_line(&In, Points, Cats, line);

	    if (!(type & only_type))
		continue;

	    if (field > 0 && !Vect_cats_in_constraint(Cats, field, cat_list))
		continue;

	    /* height attribute */
	    if (opt.hcolumn->answer) {
		cat = Vect_get_line_cat(&In, line, field);
                if (cat == -1) {
                    G_warning(_("No category defined for feature %d. Using default fixed height %f."),
                              line, objheight_default);
                    objheight = objheight_default;
                }
                if (get_height(Fi, opt.hcolumn->answer,
                               driver, cat, &objheight) != 0) {
                    G_warning(_("Unable to fetch height from DB for line %d. Using default fixed height %f."),
                              line, objheight_default);
                    objheight = objheight_default;
                }
	    } /* if opt.hcolumn->answer */
            
	    extrude(&In, &Out, Cats, Points,
		    fdrast, trace, interp_method,
                    objheight, voffset, &window, type, -1);
	} /* for each line */
    }	  /* else if area */

    if (driver) {
	db_close_database(driver);
	db_shutdown_driver(driver);
    }

    Vect_build(&Out);

    /* header */
    G_asprintf(&comment, "Generated by %s from vector map <%s>",
	       G_program_name(), Vect_get_full_name(&In));
    Vect_set_comment(&Out, comment);
    G_free(comment);

    Vect_get_map_box(&Out, &map_box);

    Vect_close(&In);
    Vect_close(&Out);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    G_done_msg("T: %f B: %f.", map_box.T, map_box.B);
    
    exit(EXIT_SUCCESS);
}
