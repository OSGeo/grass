
/****************************************************************
 *
 * MODULE:     v.hull
 *
 * AUTHOR(S):  Andrea Aime <aaime@libero.it>
 *             Updated 19 Dec 2003, Markus Neteler to 5.7
 *             Last updated 16 jan 2007, Benjamin Ducke to support 3D hull creation
 *             OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *
 * PURPOSE:    Creates the convex hull surrounding a vector points.
 *
 * COPYRIGHT:  (C) 2001-2010 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2).  Read the file COPYING that
 *             comes with GRASS for details.
 *
 ****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "hull.h"

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *input, *output, *field, *cats_opt, *where_opt;
    struct Flag *region_flag, *flat;
    struct Cell_head window;
    int layer;
    struct cat_list *cat_list = NULL;

    char *sitefile;

    struct Map_info Map;
    struct Point *points;	/* point loaded from site file */
    int *hull;			/* index of points located on the convex hull */
    int numSitePoints, numHullPoints;

    int MODE2D;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("3D"));
    module->description =
	_("Produces a 2D/3D convex hull for a given vector map.");
    
    input = G_define_standard_option(G_OPT_V_INPUT);
    
    field = G_define_standard_option(G_OPT_V_FIELD_ALL);

    output = G_define_standard_option(G_OPT_V_OUTPUT);
    
    cats_opt = G_define_standard_option(G_OPT_V_CATS);
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->description = _("Limit to current region");

    flat = G_define_flag();
    flat->key = 'f';
    flat->description =
	_("Create a 'flat' 2D hull even if the input is 3D points");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sitefile = input->answer;

    Vect_check_input_output_name(input->answer, output->answer,
				 G_FATAL_EXIT);
    
    Vect_set_open_level(1);
    if (Vect_open_old2(&Map, sitefile, "", field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), sitefile);
    
    layer = Vect_get_field_number(&Map, field->answer);
    
    if (layer > 0)
	cat_list = Vect_cats_set_constraint(&Map, layer, where_opt->answer,
                                            cats_opt->answer);
    
    /* load site coordinates */
    G_get_window(&window);
    numSitePoints = loadSiteCoordinates(&Map, &points, region_flag->answer, &window,
					layer, cat_list);
    if (numSitePoints < 0)
	G_fatal_error(_("Error loading vector points from <%s>"), sitefile);
    
    if (numSitePoints < 3)
	G_fatal_error(_("Convex hull calculation requires at least three points (%d found)"), numSitePoints);
    
    G_verbose_message(_("%d points read from vector map <%s>"), numSitePoints, sitefile);
    
    /* create a 2D or a 3D hull? */
    MODE2D = 1;
    if (Vect_is_3d(&Map)) {
	MODE2D = 0;
    }
    if (flat->answer) {
	MODE2D = 1;
    }

    /* close input vector */
    Vect_close(&Map);

    /* create output vector map */
    if (0 > Vect_open_new(&Map, output->answer, MODE2D ? WITHOUT_Z : WITH_Z)) {
	G_fatal_error(_("Unable to create vector map <%s>"), output->answer);
    }
    
    Vect_hist_command(&Map);

    if (MODE2D) {
	/* compute convex hull */
	numHullPoints = convexHull(points, numSitePoints, &hull);

	/* output vector map */
	outputHull(&Map, points, hull, numHullPoints);
    }
    else {
	/* this does everything for the 3D hull including vector map creation */
	convexHull3d(points, numSitePoints, &Map);
    }
    
    /* clean up and bye bye */
    Vect_build(&Map);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
