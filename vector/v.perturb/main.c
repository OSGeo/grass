
/*-
 * s.perturb
 * Copyright (C) 1994. James Darrell McCauley.
 *
 * Author: James Darrell McCauley darrell@mccauley-usa.com
 * 	                          http://mccauley-usa.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Modification History:
 * 3/2006              added min and seed MN/SM ITC-irst
 * 2005                updated to GRASS 6 RB ITC-irst
 * 0.1B <18 Feb 1994>  first version (jdm)
 * 0.2B <02 Jan 1995>  clean man page, added html page (jdm)
 * 0.3B <25 Feb 1995>  cleaned up 'gcc -Wall' warnings (jdm)
 * <13 Sept 2000>      released under GPL
 *
 * TODO: see code below
 */

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "perturb.h"
#include "zufall.h"

int main(int argc, char **argv)
{
    char *mapset;
    double p1, p2, numbers[1000],numbers2[1000];
    int (*rng) ();
    int i, verbose;
    int line, nlines, ttype, n, ret, seed;
    struct field_info *Fi, *Fin;
    double min=0.;
    int debuglevel=3;

    struct Map_info In, Out;

    struct line_pnts *Points;
    struct line_cats *Cats;

    struct Cell_head window;
    struct GModule *module;
    struct
    {
	struct Option *in, *out, *dist, *pars, *min, *seed;
    } parm;
    struct
    {
	struct Flag *q;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector");
    module->description = _("Random location perturbations of GRASS vector points");

    parm.in = G_define_standard_option(G_OPT_V_INPUT);
    parm.in->description = _("Vector points to be spatially perturbed");

    parm.out = G_define_standard_option(G_OPT_V_OUTPUT);

    parm.dist = G_define_option();
    parm.dist->key = "distribution";
    parm.dist->type = TYPE_STRING;
    parm.dist->required = NO;
    parm.dist->options = "uniform,normal";
    parm.dist->answer = "uniform";
    parm.dist->description = _("Distribution of perturbation");

    parm.pars = G_define_option();
    parm.pars->key = "parameters";
    parm.pars->type = TYPE_DOUBLE;
    parm.pars->required = YES;
    parm.pars->multiple = YES;
    parm.pars->description =
	_("Parameter(s) of distribution. If the distribution "
	"is uniform, only one parameter, the maximum, is needed. "
	"For a normal distribution, two parameters, the mean and "
	"standard deviation, are required.");

    parm.min = G_define_option();
    parm.min->key = "minimum";
    parm.min->type = TYPE_DOUBLE;
    parm.min->required = NO;
    parm.min->answer = "0.0";
    parm.min->description = _("Minimum deviation in map units");

    parm.seed = G_define_option();
    parm.seed->key = "seed";
    parm.seed->type = TYPE_INTEGER;
    parm.seed->required = NO;
    parm.seed->answer = "0";
    parm.seed->description = _("Seed for random number generation");

    flag.q = G_define_flag();
    flag.q->key = 'q';
    flag.q->description = _("Quiet");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    verbose = (!flag.q->answer);
    min = atof(parm.min->answer);
    seed = atoi(parm.seed->answer);

    switch (parm.dist->answer[0]) {
    case 'u':
	rng = zufall;
	break;
    case 'n':
	rng = normalen;
	break;
    }
    if (rng == zufall) {
	i = sscanf(parm.pars->answer, "%lf", &p1);
	if (i != 1) {
	    G_fatal_error(_("Error scanning arguments"));
	}
	else if (p1 <= 0)
	    G_fatal_error(_("Maximum of uniform distribution must be >= zero"));
    }
    else {
	if ((i = sscanf(parm.pars->answer, "%lf,%lf", &p1, &p2)) != 2) {
	    G_fatal_error(_("Error scanning arguments"));
	}
	if (p2 <= 0)
	    G_fatal_error (_("Standard deviation of normal distribution must be >= zero"));
    }

    G_get_window(&window);

    /* Open input */
    if ((mapset = G_find_vector2(parm.in->answer, "")) == NULL) {
	G_fatal_error(_("Vector map <%s> not found"), parm.in->answer);
    }
    Vect_set_open_level(2);
    Vect_open_old(&In, parm.in->answer, mapset);

    /* Open output */
    Vect_open_new(&Out, parm.out->answer, 0); /* TODO add z support ? */

    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* Generate a bunch of random numbers */
    zufalli(&seed);
    myrng(numbers, 1000, rng, p1 - min, p2);
    myrng(numbers2, 1000, rng, p1, p2);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(&In);

    i = 0;
    for (line = 1; line <= nlines; line++) {
	int type;

	type = Vect_read_line(&In, Points, Cats, line);

	if (type & GV_POINT) {
	    if (i >= 800) {
		/* Generate some more random numbers */
		myrng(numbers, 1000, rng, p1 - min, p2);
		myrng(numbers2, 1000, rng, p1, p2);
		i = 0;
	    }

	    G_debug(debuglevel, "x:      %f y:      %f", Points->x[0], Points->y[0]);

	    /* perturb */  /* TODO: tends to concentrate in box corners when min is used */
	    if ( numbers2[i] >= 0){
	      if ( numbers[i] >= 0){
		G_debug(debuglevel, "deltax: %f", numbers[i] + min);
		Points->x[0] += numbers[i++] + min ;
	      }else{
		G_debug(debuglevel, "deltax: %f", numbers[i] - min);
		Points->x[0] += numbers[i++] - min ;
	      }
	      Points->y[0] += numbers2[i++];
	    }else{
	      if ( numbers[i] >= 0){
		G_debug(debuglevel, "deltay: %f", numbers[i] + min);
		Points->y[0] += numbers[i++] + min ;
	      }else{ 
		G_debug(debuglevel, "deltay: %f", numbers[i] - min);
		Points->y[0] += numbers[i++] - min ;
	      }
	      Points->x[0] += numbers2[i++];
	    }

	    G_debug(debuglevel, "x_pert: %f y_pert: %f", Points->x[0], Points->y[0]);
	}

	Vect_write_line(&Out, type, Points, Cats);
    }

    /* Copy tables */
    n = Vect_get_num_dblinks(&In);
    ttype = GV_1TABLE;
    if (n > 1)
	ttype = GV_MTABLE;

    for (i = 0; i < n; i++) {
	Fi = Vect_get_dblink(&In, i);
	if (Fi == NULL) {
	    G_fatal_error(_("Cannot get db link info"));
	}
	Fin = Vect_default_field_info(&Out, Fi->number, Fi->name, ttype);
	Vect_map_add_dblink(&Out, Fi->number, Fi->name, Fin->table, Fi->key,
			    Fin->database, Fin->driver);

	ret = db_copy_table(Fi->driver, Fi->database, Fi->table,
			    Fin->driver, Vect_subst_var(Fin->database, &Out),
			    Fin->table);
	if (ret == DB_FAILED) {
	    G_warning("Cannot copy table");
	}
    }

    Vect_close(&In);

    Vect_build(&Out, stderr);
    Vect_close(&Out);

    return (EXIT_SUCCESS);
}
