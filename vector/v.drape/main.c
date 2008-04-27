/**********************************************************
 *
 * MODULE:       v.drape
 * 
 * AUTHOR(S):    Radim Blazek, Dylan Beaudette
 *               
 * PURPOSE:      Convert 2D vector to 3D vector by sampling of elevation raster.
 *               
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 * 
 **********************************************************/


/** Doxygen Style Comments
\file main.c
\brief v.drape module for converting 2D vectors into 3D vectors by means of sampling an elevation raster.
 
\author Radim Blazek
\author Dylan Beaudette
\date 2005.09.20
 
\todo add support for areas
*/

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *out_opt, *type_opt, *rast_opt, *method_opt, *scale_opt;

    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    /* int    layer; */
    int line, nlines, otype, ltype;
    BOUND_BOX in_bbox, region_bbox, rast_bbox;

    char *mapset;
    int j;
    double scale, estimated_elevation;
    INTERP_TYPE method = UNKNOWN;
    int fdrast;			/* file descriptor for raster map is int */
    struct Cell_head window, rast_window;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, geometry, sampling");
    module->description =
      _("Converts vector map to 3D by sampling of elevation raster map.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,centroid,line,boundary,face,kernel";
    type_opt->answer = "point,centroid,line,boundary,face,kernel";

    /* raster sampling */
    rast_opt = G_define_standard_option(G_OPT_R_MAP);
    rast_opt->key = "rast";
    rast_opt->required = NO;
    rast_opt->description = _("Elevation raster map for height extraction");

    scale_opt = G_define_option();
    scale_opt->key = "scale";
    scale_opt->type = TYPE_DOUBLE;
    scale_opt->description = _("Scale sampled raster values");
    scale_opt->answer = "1.0";

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->multiple = NO;
    method_opt->options = "nearest,bilinear,cubic";
    method_opt->answer = "nearest";
    method_opt->descriptions = "nearest;nearest neighbor;"
			"bilinear;bilinear interpolation;"
			"cubic;cubic convolution interpolation;";
    method_opt->description = _("Sampling method");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* which interpolation method should we use */
    if ( method_opt->answer[0] == 'b' )
	method = BILINEAR;
    else if ( method_opt->answer[0] == 'c' )
	method = CUBIC;
    else
    {
        method = NEAREST;
    }

    /* setup the raster for sampling */

    /* setup the region */
    G_get_window(&window);

    /* used to scale sampled raster values*/
    scale = atof(scale_opt->answer);

    /* Check output type */
    otype = Vect_option_to_types(type_opt);

    /* check for the elev raster, and check for error condition */
    if ((mapset = G_find_cell(rast_opt->answer, "")) == NULL) {
	G_fatal_error(_("Raster map <%s> not found"), rast_opt->answer);
    }

    /* open the elev raster, and check for error condition */
    if ((fdrast = G_open_cell_old(rast_opt->answer, mapset)) < 0) {
	G_fatal_error(_("Unable to open raster map <%s>"), rast_opt->answer);
    }

    /* read raster header */
    G_get_cellhd(rast_opt->answer, mapset, &rast_window);

    Vect_set_open_level(2);

    /* check input/output vector maps */
    Vect_check_input_output_name(in_opt->answer, out_opt->answer, GV_FATAL_EXIT);

    mapset = G_find_vector2(in_opt->answer, "");
    if (!mapset) {
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);
    }

    Vect_open_old(&In, in_opt->answer, mapset);

    /* checks 
       does the elevation raster cover the entire are of the vector map?
       does the current region include the entire input vector map ?
    */
    Vect_get_map_box(&In, &in_bbox);
    Vect_region_box(&window, &region_bbox);
    Vect_region_box(&rast_window, &rast_bbox);
    if (in_bbox.W < region_bbox.W ||
	in_bbox.E > region_bbox.E ||
	in_bbox.S < region_bbox.S ||
	in_bbox.N > region_bbox.N) {
	G_warning (_("Current region does not include the entire input vector map <%s>"),
		   in_opt->answer);
    }
    if (in_bbox.W < rast_bbox.W ||
	in_bbox.E > rast_bbox.E ||
	in_bbox.S < rast_bbox.S ||
	in_bbox.N > rast_bbox.N) {	
	G_warning (_("Elevation raster map <%s> does not cover the entire area "
		     "of the input vector map <%s>. "),
		   rast_opt->answer, in_opt->answer);
    }

    /* setup the new vector map */
    /* remember to open the new vector map as 3D */
    Vect_open_new(&Out, out_opt->answer, WITH_Z);
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    /* copy the input vector's attribute table to the new vector */
    /* This works for both level 1 and 2 */
    Vect_copy_tables(&In, &Out, 0);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* line types */
    if ((otype &
	 (GV_POINTS | GV_LINES | GV_BOUNDARY | GV_CENTROID | GV_FACE |
	  GV_KERNEL))) {

	/* loop through each line in the dataset */
	nlines = Vect_get_num_lines(&In);

	for (line = 1; line <= nlines; line++) {

	    /* progress feedback */
	    G_percent(line, nlines, 2);

	    /* get the line type */
	    ltype = Vect_read_line(&In, Points, Cats, line);

	    /* adjust flow based on specific type of line */
	    switch (ltype) {
		/* points (at least 1 vertex) */
	    case GV_POINT:
	    case GV_CENTROID:
	    case GV_KERNEL:
                /* sample raster at this point, and update the z-coordinate
                 * (note that input vector should not be 3D!)
                 */
                estimated_elevation = scale * G_get_raster_sample(fdrast,
                            &window, NULL, Points->y[0], Points->x[0], 0, method);

		/* update the elevation value for each data point */
		Points->z[0] = estimated_elevation;
		break;
		/* standard lines (at least 2 vertexes) */
	    case GV_LINE:
	    case GV_BOUNDARY:
		if (Points->n_points < 2)
		    break;	/* At least 2 points */

		/* loop through each point in a line */
		for (j = 0; j < Points->n_points; j++) {
		    /* sample raster at this point, and update the z-coordinate (note that input vector should not be 3D!) */
                    estimated_elevation = scale * G_get_raster_sample(fdrast,
                                &window, NULL, Points->y[j], Points->x[j], 0, method);

		    /* update the elevation value for each data point */
		    Points->z[j] = estimated_elevation;
		}		/* end looping through point in a line */
		break;

		/* lines with at least 3 vertexes */
	    case GV_FACE:
		if (Points->n_points < 3)
		    break;	/* At least 3 points */

		/* loop through each point in a line */
		for (j = 0; j < Points->n_points; j++) {
		    /* sample raster at this point, and update the z-coordinate (note that input vector should not be 3D!) */
                    estimated_elevation = scale * G_get_raster_sample(fdrast,
                                &window, NULL, Points->y[j], Points->x[j], 0, method);

		    /* update the elevation value for each data point */
		    Points->z[j] = estimated_elevation;
		}
		break;
	    }			/* end line type switch */

	    /* write the new line file, with the updated Points struct*/
	    Vect_write_line(&Out, ltype, Points, Cats);
	}			/* end looping thru lines */

    }				/* end working on type=lines */

    /* close elevation raster: */
    G_close_cell(fdrast);

    /* close input vector */
    Vect_close(&In);
    /* build topology for output vector */
    if (G_verbose() > G_verbose_min()) {
	Vect_build(&Out, stderr);
    }
    else {
	Vect_build(&Out, NULL);
    }

    /* close output vector */
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
