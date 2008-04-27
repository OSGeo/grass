/****************************************************************
 *
 * MODULE:       v.random (based on s.rand)
 *
 * AUTHOR(S):    James Darrell McCauley darrell@mccauley-usa.com
 * 	         http://mccauley-usa.com/
 *
 * PURPOSE:      Randomly generate a 2D/3D GRASS vector points map.
 *
 * COPYRIGHT:    (C) 2003-2007 by the GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
**************************************************************/
/*
 * Modification History:
 *
 * s.rand v 0.5B <25 Jun 1995> Copyright (c) 1993-1995. James Darrell McCauley
 * <?? ??? 1993> - began coding and released test version (jdm)
 * <10 Jan 1994> - changed RAND_MAX for rand(), since it is different for
 *                 SunOS 4.1.x and Solaris 2.3. stdlib.h in the latter defines
 *                 RAND_MAX, but it doesn't in the former. v0.2B (jdm)
 * <02 Jan 1995> - clean Gmakefile, man page. added html v0.3B (jdm)
 * <25 Feb 1995> - cleaned 'gcc -Wall' warnings (jdm)
 * <25 Jun 1995> - new site API (jdm)
 * <13 Sep 2000> - released under GPL
 */

#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#ifndef RAND_MAX
#define RAND_MAX (pow(2.0,31.0)-1)
#endif
double myrand(void);

#if defined(__CYGWIN__) || defined(__APPLE__) || defined(__MINGW32__)
double drand48()
{
    return (rand() / 32767.0);
}

#define srand48(sv) (srand((unsigned)(sv)))
#endif

int main(int argc, char *argv[])
{
    char *output;
    double (*rng) (), max;
    int i, n, b;
    struct Map_info Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct Cell_head window;
    struct GModule *module;
    struct
    {
	struct Option *output, *nsites, *zmin, *zmax;
    } parm;
    struct
    {
	struct Flag *rand, *drand48, *z, *notopo;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, statistics");
    module->description =
	_("Randomly generate a 2D/3D vector points map.");

    parm.output = G_define_standard_option(G_OPT_V_OUTPUT);

    parm.nsites = G_define_option();
    parm.nsites->key = "n";
    parm.nsites->type = TYPE_INTEGER;
    parm.nsites->required = YES;
    parm.nsites->description = _("Number of points to be created");

    parm.zmin = G_define_option();
    parm.zmin->key = "zmin";
    parm.zmin->type = TYPE_DOUBLE;
    parm.zmin->required = NO;
    parm.zmin->description = _("Minimum z height (needs -z flag)");
    parm.zmin->answer = "0.0";

    parm.zmax = G_define_option();
    parm.zmax->key = "zmax";
    parm.zmax->type = TYPE_DOUBLE;
    parm.zmax->required = NO;
    parm.zmax->description = _("Maximum z height (needs -z flag)");
    parm.zmax->answer = "0.0";

    flag.z = G_define_flag();
    flag.z->key = 'z';
    flag.z->description = _("Create 3D output");

    flag.drand48 = G_define_flag();
    flag.drand48->key = 'd';
    flag.drand48->description = _("Use drand48() function instead of rand()");

    flag.notopo = G_define_flag();
    flag.notopo->key          = 'b';
    flag.notopo->description  = _("Do not build topology");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    output = parm.output->answer;
    n = atoi(parm.nsites->answer);
    b = (flag.drand48->answer == '\0') ? 0 : 1;

    if (G_legal_filename(output) == -1) {
	G_fatal_error(_("<%s> is an illegal file name"),
		      output);
    }

    if (n <= 0) {
	G_fatal_error(_("Number of points must be > 0 (%d given)"),
		      n);
    }

    if (flag.z->answer)
	Vect_open_new(&Out, output, WITH_Z);
    else
	Vect_open_new(&Out, output, WITHOUT_Z);

    Vect_hist_command(&Out);

    if (b) {
	rng = drand48;
	max = 1.0;
	srand48((long)getpid());
    }
    else {			/* default is rand() */

	rng = myrand;
	max = RAND_MAX;
	srand(getpid());
    }

    G_get_window(&window);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    G_important_message(_("Generating points..."));
    for (i = 0; i < n; ++i) {
	double x, y, z;

	G_percent(i, n, 5);

	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	x = rng() / max * (window.west - window.east) + window.east;
	y = rng() / max * (window.north - window.south) + window.south;


	if (flag.z->answer) {
	    z = rng() / max * (atof(parm.zmax->answer) -
			       atof(parm.zmin->answer)) +
		atof(parm.zmin->answer);
	    Vect_append_point(Points, x, y, z);
	}
	else
	    Vect_append_point(Points, x, y, 0.0);

	Vect_cat_set(Cats, 1, i + 1);
	Vect_write_line(&Out, GV_POINT, Points, Cats);
    }

    if (!flag.notopo->answer) {
	if (G_verbose() > G_verbose_min())
	    Vect_build(&Out, stderr);
	else
	    Vect_build(&Out, NULL);
    }
    Vect_close(&Out);

    exit (EXIT_SUCCESS);
}

double myrand()
{
    return (double)rand();
}
