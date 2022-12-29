/****************************************************************************
 *
 * MODULE:       v.qcount
 * AUTHOR(S):    James Darrell McCauley darrell@mccauley-usa.com
 * 	                          http://mccauley-usa.com/
 *               OGR support by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      GRASS program to sample a raster map at site locations (based on s.qcount)
 *
 * Modification History:
 * <03 Mar 1993> - began coding (jdm)
 * <11 Jan 1994> - announced version 0.3B on pasture.ecn.purdue.edu (jdm)
 * <14 Jan 1994> - v 0.4B, corrected some spelling errors (jdm)
 * <02 Jan 1995> - v 0.5B, clean Gmakefile, man page, added html (jdm)
 * <25 Feb 1995> - v 0.6B, cleaned 'gcc -Wall' warnings (jdm)
 * <25 Jun 1995> - v 0.7B, new site API (jdm)
 * <13 Sep 2000> - released under GPL
 *
 * COPYRIGHT:    (C) 2003-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "quaddefs.h"

int main(int argc, char **argv)
{
    double radius;
    double fisher, david, douglas, lloyd, lloydip, morisita;
    int i, nquads, *counts;

    struct Cell_head window;
    struct GModule *module;
    struct
    {
	struct Option *input, *field, *output, *n, *r;
    } parm;
    struct
    {
	struct Flag *g;
    } flag;
    COOR *quads;

    struct Map_info Map;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("point pattern"));
    module->description = _("Indices for quadrat counts of vector point lists.");

    parm.input = G_define_standard_option(G_OPT_V_INPUT);

    parm.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    
    parm.output = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.output->required = NO;
    parm.output->description =
	_("Name for output quadrat centers map (number of points is written as category)");

    parm.n = G_define_option();
    parm.n->key = "nquadrats";
    parm.n->type = TYPE_INTEGER;
    parm.n->required = YES;
    parm.n->description = _("Number of quadrats");

    parm.r = G_define_option();
    parm.r->key = "radius";
    parm.r->type = TYPE_DOUBLE;
    parm.r->required = YES;
    parm.r->description = _("Quadrat radius");

    flag.g = G_define_flag();
    flag.g->key = 'g';
    flag.g->description = _("Print results in shell script style");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sscanf(parm.n->answer, "%d", &nquads);
    sscanf(parm.r->answer, "%lf", &radius);

    G_get_window(&window);

    /* Open input */
    Vect_set_open_level(2);
    if (Vect_open_old2(&Map, parm.input->answer, "", parm.field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), parm.input->answer);

    /* Get the quadrats */
    G_message(_("Finding quadrats..."));

    quads = find_quadrats(nquads, radius, window);

    /* Get the counts per quadrat */
    G_message(_("Counting points quadrats..."));

    counts = (int *)G_malloc(nquads * (sizeof(int)));
    count_sites(quads, nquads, counts, radius, &Map,
		Vect_get_field_number(&Map, parm.field->answer));

    Vect_close(&Map);

    /* output if requested */
    if (parm.output->answer) {
	struct Map_info Out;
	struct line_pnts *Points;
	struct line_cats *Cats;

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

	if (Vect_open_new(&Out, parm.output->answer, 0) < 0)
	    G_fatal_error(_("Unable to create vector map <%s>"),
			    parm.output->answer);

	Vect_hist_command(&Out);

	for (i = 0; i < nquads; i++) {
	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);

	    Vect_append_point(Points, quads[i].x, quads[i].y, 0.0);
	    Vect_cat_set(Cats, 1, counts[i]);

	    Vect_write_line(&Out, GV_POINT, Points, Cats);
	}

	Vect_build(&Out);
	Vect_close(&Out);

    }

    /* Indices if requested */
    qindices(counts, nquads, &fisher, &david, &douglas, &lloyd, &lloydip,
	     &morisita);

    if (!flag.g->answer) {
	fprintf(stdout,
		"-----------------------------------------------------------\n");
	fprintf(stdout,
		"Index                                           Realization\n");
	fprintf(stdout,
		"-----------------------------------------------------------\n");
	fprintf(stdout,
		"Fisher el al (1922) Relative Variance            %g\n",
		fisher);
	fprintf(stdout,
		"David & Moore (1954) Index of Cluster Size       %g\n",
		david);
	fprintf(stdout,
		"Douglas (1975) Index of Cluster Frequency        %g\n",
		douglas);
	fprintf(stdout,
		"Lloyd (1967) \"mean crowding\"                     %g\n",
		lloyd);
	fprintf(stdout,
		"Lloyd (1967) Index of patchiness                 %g\n",
		lloydip);
	fprintf(stdout,
		"Morisita's (1959) I (variability b/n patches)    %g\n",
		morisita);
	fprintf(stdout,
		"-----------------------------------------------------------\n");
    }
    else {
	fprintf(stdout, "fisher=%g\n", fisher);
	fprintf(stdout, "david=%g\n", david);
	fprintf(stdout, "douglas=%g\n", douglas);
	fprintf(stdout, "lloyd=%g\n", lloyd);
	fprintf(stdout, "lloydip=%g\n", lloydip);
	fprintf(stdout, "morisita=%g\n", morisita);
    }



    exit(EXIT_SUCCESS);
}
