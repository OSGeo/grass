/***************************************************************
 *
 * MODULE:       v.normal
 * 
 * AUTHOR(S):    James Darrell McCauley darrell@mccauley-usa.com
 *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *               
 * PURPOSE:      GRASS program for distributional testing (based on s.normal)
 *               
 * COPYRIGHT:    (C) 2001-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details. 
 * Modification History:
 * <23 Jan 2001> - added field parameter, fixed reading of sites (MN)
 * <27 Aug 1994> - began coding. Adapted cdh.f from statlib (jdm)
 * <30 Sep 1994> - finished alpha version of cdh-c (jdm)
 * <10 Oct 1994> - announced version 0.1B on pasture.ecn.purdue.edu (jdm)
 * <02 Jan 1995> - cleaned man page, added html page (jdm)
 * <25 Feb 1995> - cleaned 'gcc -Wall' warnings (jdm)
 * <21 Jun 1995> - adapted to use new sites API (jdm)
 * <13 Sep 2000> - released under GPL
 *
 **************************************************************/

#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/cdhc.h>
#include <grass/glocale.h>

int scan_cats(char *, long *, long *);

int main(int argc, char **argv)
{
    int i, nsites, warn_once = 0;
    int all;
    long x, y;
    struct Cell_head window;
    struct GModule *module;
    struct
    {
	struct Option *input, *tests, *dfield, *layer;
    } parm;
    struct
    {
	struct Flag *q, *l, *region;
    } flag;
    double *w, *z;

    struct Map_info Map;
    int line, nlines, npoints;
    int field;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct bound_box box;

    /* Attributes */
    int nrecords;
    int ctype;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("points"));
    G_add_keyword(_("point pattern"));
    module->description = _("Tests for normality for vector points.");

    parm.input = G_define_standard_option(G_OPT_V_MAP);

    parm.layer = G_define_standard_option(G_OPT_V_FIELD);
    
    parm.tests = G_define_option();
    parm.tests->key = "tests";
    parm.tests->key_desc = "range";
    parm.tests->type = TYPE_STRING;
    parm.tests->multiple = YES;
    parm.tests->required = YES;
    parm.tests->label = _("Lists of tests (1-15)");
    parm.tests->description = _("E.g. 1,3-8,13");

    parm.dfield = G_define_standard_option(G_OPT_DB_COLUMN);
    parm.dfield->required = YES;

    flag.region = G_define_flag();
    flag.region->key = 'r';
    flag.region->description = _("Use only points in current region");

    flag.l = G_define_flag();
    flag.l->key = 'l';
    flag.l->description = _("Lognormality instead of normality");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    all = flag.region->answer ? 0 : 1;

    /* Open input */
    Vect_set_open_level(2);
    if (Vect_open_old2(&Map, parm.input->answer, "", parm.layer->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), parm.input->answer);

    field = Vect_get_field_number(&Map, parm.layer->answer);
    
    /* Read attributes */
    Fi = Vect_get_field(&Map, field);
    if (Fi == NULL) {
	G_fatal_error("Database connection not defined for layer %d", field);
    }
    
    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    nrecords = db_select_CatValArray(Driver, Fi->table, Fi->key, parm.dfield->answer,
				     NULL, &cvarr);
    G_debug(1, "nrecords = %d", nrecords);

    ctype = cvarr.ctype;
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Only numeric column type supported"));

    if (nrecords < 0)
	G_fatal_error(_("Unable to select data from table"));
    G_verbose_message(_("%d records selected from table"), nrecords);

    db_close_database_shutdown_driver(Driver);

    /* Read points */
    npoints = Vect_get_num_primitives(&Map, GV_POINT);
    z = (double *)G_malloc(npoints * sizeof(double));

    G_get_window(&window);
    Vect_region_box(&window, &box);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(&Map);
    nsites = 0;
    for (line = 1; line <= nlines; line++) {
	int type, cat, ret, cval;
	double dval;

	G_debug(3, "line = %d", line);

	type = Vect_read_line(&Map, Points, Cats, line);
	if (!(type & GV_POINT))
	    continue;

	if (!all) {
	    if (!Vect_point_in_box(Points->x[0], Points->y[0], 0.0, &box))
		continue;
	}

	Vect_cat_get(Cats, 1, &cat);

	G_debug(3, "cat = %d", cat);

	/* find actual value */
	if (ctype == DB_C_TYPE_INT) {
	    ret = db_CatValArray_get_value_int(&cvarr, cat, &cval);
	    if (ret != DB_OK) {
		G_warning(_("No record for cat %d"), cat);
		continue;
	    }
	    dval = cval;
	}
	else if (ctype == DB_C_TYPE_DOUBLE) {
	    ret = db_CatValArray_get_value_double(&cvarr, cat, &dval);
	    if (ret != DB_OK) {
		G_warning(_("No record for cat %d"), cat);
		continue;
	    }
	}

	G_debug(3, "dval = %e", dval);
	z[nsites] = dval;
	nsites++;
    }

    G_verbose_message(_("Number of points: %d"), nsites);
    
    if (nsites <= 0)
	G_fatal_error(_("No points found"));

    if (nsites < 4)
	G_warning(_("Too small sample"));
    
    if (flag.l->answer) {
	warn_once = 0;
	for (i = 0; i < nsites; ++i) {
	    if (z[i] > 1.0e-10)
		z[i] = log10(z[i]);
	    else if (!warn_once) {
		G_warning(_("Negative or very small point values set to -10.0"));
		z[i] = -10.0;
		warn_once = 1;
	    }
	}
    }

    for (i = 0; parm.tests->answers[i]; i++)
	if (!scan_cats(parm.tests->answers[i], &x, &y)) {
	    G_usage();
	    exit(EXIT_FAILURE);
	}
    for (i = 0; parm.tests->answers[i]; i++) {
	scan_cats(parm.tests->answers[i], &x, &y);
	while (x <= y)
	    switch (x++) {
	    case 1:		/* moments */
		fprintf(stdout, _("Moments \\sqrt{b_1} and b_2: "));
		w = Cdhc_omnibus_moments(z, nsites);
		fprintf(stdout, "%g %g\n", w[0], w[1]);
		break;
	    case 2:		/* geary */
		fprintf(stdout, _("Geary's a-statistic & an approx. normal: "));
		w = Cdhc_geary_test(z, nsites);
		fprintf(stdout, "%g %g\n", w[0], w[1]);
		break;
	    case 3:		/* extreme deviates */
		fprintf(stdout, _("Extreme normal deviates: "));
		w = Cdhc_extreme(z, nsites);
		fprintf(stdout, "%g %g\n", w[0], w[1]);
		break;
	    case 4:		/* D'Agostino */
		fprintf(stdout, _("D'Agostino's D & an approx. normal: "));
		w = Cdhc_dagostino_d(z, nsites);
		fprintf(stdout, "%g %g\n", w[0], w[1]);
		break;
	    case 5:		/* Kuiper */
		fprintf(stdout,
			_("Kuiper's V (regular & modified for normality): "));
		w = Cdhc_kuipers_v(z, nsites);
		fprintf(stdout, "%g %g\n", w[1], w[0]);
		break;
	    case 6:		/* Watson */
		fprintf(stdout,
			_("Watson's U^2 (regular & modified for normality): "));
		w = Cdhc_watson_u2(z, nsites);
		fprintf(stdout, "%g %g\n", w[1], w[0]);
		break;
	    case 7:		/* Durbin */
		fprintf(stdout,
			_("Durbin's Exact Test (modified Kolmogorov): "));
		w = Cdhc_durbins_exact(z, nsites);
		fprintf(stdout, "%g\n", w[0]);
		break;
	    case 8:		/* Anderson-Darling */
		fprintf(stdout,
			_("Anderson-Darling's A^2 (regular & modified for normality): "));
		w = Cdhc_anderson_darling(z, nsites);
		fprintf(stdout, "%g %g\n", w[1], w[0]);
		break;
	    case 9:		/* Cramer-Von Mises */
		fprintf(stdout,
			_("Cramer-Von Mises W^2(regular & modified for normality): "));
		w = Cdhc_cramer_von_mises(z, nsites);
		fprintf(stdout, "%g %g\n", w[1], w[0]);
		break;
	    case 10:		/* Kolmogorov-Smirnov */
		fprintf(stdout,
			_("Kolmogorov-Smirnov's D (regular & modified for normality): "));
		w = Cdhc_kolmogorov_smirnov(z, nsites);
		fprintf(stdout, "%g %g\n", w[1], w[0]);
		break;
	    case 11:		/* chi-square */
		fprintf(stdout,
			_("Chi-Square stat (equal probability classes) and d.f.: "));
		w = Cdhc_chi_square(z, nsites);
		fprintf(stdout, "%g %d\n", w[0], (int)w[1]);
		break;
	    case 12:		/* Shapiro-Wilk */
		if (nsites > 50) {
		    G_warning(_("Shapiro-Wilk's W cannot be used for n > 50"));
		    if (nsites < 99)
			G_message(_("Use Weisberg-Binghams's W''"));
		}
		else {
		    fprintf(stdout, _("Shapiro-Wilk W: "));
		    w = Cdhc_shapiro_wilk(z, nsites);
		    fprintf(stdout, "%g\n", w[0]);
		}
		break;
	    case 13:		/* Weisberg-Bingham */
		if (nsites > 99 || nsites < 50)
		    G_warning(_("Weisberg-Bingham's W'' cannot be used for n < 50 or n > 99"));
		else {
		    fprintf(stdout, _("Weisberg-Bingham's W'': "));
		    w = Cdhc_weisberg_bingham(z, nsites);
		    fprintf(stdout, "%g\n", w[0]);
		}
		break;
	    case 14:		/* Royston */
		if (nsites > 2000)
		    G_warning(_("Royston only extended Shapiro-Wilk's W up to n = 2000"));
		else {
		    fprintf(stdout, _("Shapiro-Wilk W'': "));
		    w = Cdhc_royston(z, nsites);
		    fprintf(stdout, "%g\n", w[0]);
		}
		break;
	    case 15:		/* Kotz */
		fprintf(stdout, _("Kotz' T'_f (Lognormality vs. Normality): "));
		w = Cdhc_kotz_families(z, nsites);
		fprintf(stdout, "%g\n", w[0]);
		break;
	    default:
		break;
	    }
    }
    exit(EXIT_SUCCESS);
}
