
/**********************************************************
 *
 * MODULE:       v.drape
 * 
 * AUTHOR(S):    Radim Blazek, Dylan Beaudette
 *               Maris Nartiss - WHERE support and raster NULL support
 *               OGR support & major rewrite for GRASS 7 by Martin
 *               Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Convert 2D vector to 3D vector by sampling of elevation raster.
 *               
 * COPYRIGHT:    (C) 2005-2009, 2013 by the GRASS Development Team
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

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
        struct Option *input, *output, *type, *rast, *method,
            *scale, *where, *layer, *null, *cats;
    } opt;
    
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct cat_list *cat_list;

    int otype, layer;
    double scale, null_val;
    INTERP_TYPE method;
    int fdrast;			/* file descriptor for raster map is int */
    int nlines, line, ltype;

    struct Cell_head window;
    struct bound_box map_box;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("3D"));
    module->description =
	_("Converts 2D vector features to 3D by sampling of elevation raster map.");

    opt.input = G_define_standard_option(G_OPT_V_INPUT);

    opt.layer = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt.layer->guisection = _("Selection");
    
    opt.cats = G_define_standard_option(G_OPT_V_CATS);
    opt.cats->guisection = _("Selection");

    opt.where = G_define_standard_option(G_OPT_DB_WHERE);
    opt.where->guisection = _("Selection");

    opt.type = G_define_standard_option(G_OPT_V_TYPE);
    opt.type->options = "point,line,boundary,centroid";
    opt.type->answer = "point,line,boundary,centroid";
    opt.type->guisection = _("Selection");

    opt.output = G_define_standard_option(G_OPT_V_OUTPUT);

    opt.rast = G_define_standard_option(G_OPT_R_ELEV);
    opt.rast->description = _("Elevation raster map for height extraction");

    opt.method = G_define_standard_option(G_OPT_R_INTERP_TYPE);
    opt.method->answer = "nearest";
    opt.method->guisection = _("Elevation");

    opt.scale = G_define_option();
    opt.scale->key = "scale";
    opt.scale->type = TYPE_DOUBLE;
    opt.scale->description = _("Scale factor sampled raster values");
    opt.scale->answer = "1.0";
    opt.scale->guisection = _("Elevation");

    opt.null = G_define_option();
    opt.null->key = "null_value";
    opt.null->type = TYPE_DOUBLE;
    opt.null->description =
	_("Height for sampled raster NULL values");
    opt.null->guisection = _("Elevation");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    /* which interpolation method should we use */
    method = Rast_option_to_interp_type(opt.method);
    
    /* setup the region */
    G_get_window(&window);

    /* used to scale sampled raster values */
    scale = atof(opt.scale->answer);

    /* is null value defined */
    if (opt.null->answer)
	null_val = atof(opt.null->answer);

    /* check output type */
    otype = Vect_option_to_types(opt.type);
    if (otype & GV_AREA) { 
        if (otype & GV_BOUNDARY) { 
            /* process area -> skip boundaries */
            otype &= ~GV_BOUNDARY; 
        }
        if (otype & GV_CENTROID) { 
            /* process area -> skip centroids */
            otype &= ~GV_CENTROID; 
        }
    }
    
    /* open the elev raster map */
    fdrast = Rast_open_old(opt.rast->answer, "");
    
    /* check input/output vector maps */
    Vect_check_input_output_name(opt.input->answer, opt.output->answer,
				 G_FATAL_EXIT);
    
    /* open input vector map */
    Vect_set_open_level(2); /* topology required ? */
    Vect_open_old2(&In, opt.input->answer, "", opt.layer->answer);
    Vect_set_error_handler_io(&In, &Out);
    
    /* get layer number */
    layer = Vect_get_field_number(&In, opt.layer->answer);
    if ((opt.cats->answer || opt.where->answer) && layer == -1) {
        G_warning(_("Invalid layer number (%d). "
                    "Parameter '%s' or '%s' specified, assuming layer '1'."),
                  layer, opt.cats->key, opt.where->key);
        layer = 1;
    }

    /* create output */
    Vect_open_new(&Out, opt.output->answer, WITH_Z);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    
    /* set constraint for cats or where options */
    cat_list = NULL;
    if (layer > 0)
	cat_list = Vect_cats_set_constraint(&In, layer, opt.where->answer,
                                            opt.cats->answer);
    
    /* allocate space for points and cats */
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* loop through each line  */
    nlines = Vect_get_num_lines(&In);
    G_important_message(_("Processing features..."));
    for (line = 1; line <= nlines; line++) {
        /* progress feedback */
        G_percent(line, nlines, 2);
        
        if (!Vect_line_alive(&In, line))
            continue;
        
        /* get the line type */
        ltype = Vect_read_line(&In, Points, Cats, line);
        if (!(ltype & otype))
            continue;
        if (layer > 0 && !Vect_cats_in_constraint(Cats, layer, cat_list))
            continue;
        
        /* write the new line file, with the updated Points struct */
        if (sample_raster(fdrast, &window, Points, 
                          method, scale,
                          opt.null->answer ? TRUE : FALSE, null_val)) {
            Vect_write_line(&Out, ltype, Points, Cats);
        }
        else {
            G_warning(_("Undefined height for feature %d. Skipping."), line);
        }
    }
    
    /* copy attribute data */
    G_important_message(_("Copying attribute tables..."));
    if (layer < 0)
        Vect_copy_tables(&In, &Out, 0);
    else 
        Vect_copy_table_by_cat_list(&In, &Out, layer, layer, NULL,
                                    GV_1TABLE, cat_list);
    
    /* build topology for output vector */
    Vect_build(&Out);
        
    Vect_get_map_box(&Out, &map_box);

    /* close elevation raster map */
    Rast_close(fdrast);
    /* close input vector map */
    Vect_close(&In);
    /* close output vector map */
    Vect_close(&Out);

    G_done_msg("T: %f B: %f.", map_box.T, map_box.B);
    
    exit(EXIT_SUCCESS);
}
