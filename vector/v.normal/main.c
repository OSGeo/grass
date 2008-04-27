/*-
 * from s.normal - GRASS program for distributional testing.
 * Copyright (C) 1994-1995. James Darrell McCauley.
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
 * <23 Jan 2001> - added field parameter, fixed reading of sites (MN)
 * <27 Aug 1994> - began coding. Adapted cdh.f from statlib (jdm)
 * <30 Sep 1994> - finished alpha version of cdh-c (jdm)
 * <10 Oct 1994> - announced version 0.1B on pasture.ecn.purdue.edu (jdm)
 * <02 Jan 1995> - cleaned man page, added html page (jdm)
 * <25 Feb 1995> - cleaned 'gcc -Wall' warnings (jdm)
 * <21 Jun 1995> - adapted to use new sites API (jdm)
 * <13 Sep 2000> - released under GPL
 *
 */

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/cdhc.h>
#include <grass/glocale.h>

int scan_cats ( char *, long *, long *);

int main (int argc, char **argv)
{
  char   *mapset;
  int    i, nsites, verbose, warn_once = 0;
  int    all;
  long   x, y;
  struct Cell_head window;
  struct GModule *module;
  struct
  {
    struct Option *input, *tests, *dfield;
  } parm;
  struct
  {
    struct Flag *q, *l, *region;
  } flag;
  double *w, *z;

  struct Map_info Map;
  int line, nlines, npoints;
  struct line_pnts *Points;
  struct line_cats *Cats;
  BOUND_BOX box;

  /* Attributes */
  int nrecords;
  int ctype;
  struct field_info *Fi;
  dbDriver *Driver;
  dbCatValArray cvarr;

  G_gisinit (argv[0]);

  module = G_define_module();
  module->keywords = _("vector, statistics");
  module->description = _("Tests for normality for points.");
  parm.input = G_define_option ();
  parm.input->key = "map";
  parm.input->type = TYPE_STRING;
  parm.input->required = YES;
  parm.input->description = "point vector defining sample points";
  parm.input->gisprompt = "old,vector,vector";

  parm.tests = G_define_option ();
  parm.tests->key = "tests";
  parm.tests->key_desc = "range";
  parm.tests->type = TYPE_STRING;
  parm.tests->multiple = YES;
  parm.tests->required = YES;
  parm.tests->description = "Lists of tests (1-15): e.g. 1,3-8,13";

  parm.dfield = G_define_option ();
  parm.dfield->key = "column";
  parm.dfield->type = TYPE_STRING;
  parm.dfield->multiple = NO;
  parm.dfield->required = YES;
  parm.dfield->description = "Attribute column";

  flag.region = G_define_flag ();
  flag.region->key = 'r';
  flag.region->description = "Use only points in current region";
      
  flag.q = G_define_flag ();
  flag.q->key = 'q';
  flag.q->description = "Quiet";

  flag.l = G_define_flag ();
  flag.l->key = 'l';
  flag.l->description = "lognormal";

  if (G_parser (argc, argv))
    exit (EXIT_FAILURE);

  if (parm.tests->answer == NULL)
  {
    G_usage ();
    exit (EXIT_FAILURE);
  }

  all = flag.region->answer ? 0 : 1;
  verbose = (flag.q->answer == (char) NULL) ? 1 : 0;

  /* Open input */
  if ((mapset = G_find_vector2 (parm.input->answer, "")) == NULL) {
     G_fatal_error ( _("Vector map <%s> not found"), parm.input->answer);
  }
  Vect_set_open_level (2);
  Vect_open_old ( &Map, parm.input->answer, mapset );

  /* Read attributes */
  Fi = Vect_get_field( &Map, 1);
  if ( Fi == NULL ) {
      G_fatal_error ("Cannot read layer info");
  }

  Driver = db_start_driver_open_database ( Fi->driver, Fi->database );
  if (Driver == NULL)
    G_fatal_error(_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);
  
  nrecords = db_select_CatValArray ( Driver, Fi->table, Fi->key, parm.dfield->answer, NULL, &cvarr );
  G_debug (1, "nrecords = %d", nrecords );

  ctype = cvarr.ctype;
  if ( ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE )
      G_fatal_error ( "Column type not supported" );

  if ( nrecords < 0 ) G_fatal_error (_("Unable to select data from table"));
  G_message (_( "%d records selected from table"), nrecords);

  db_close_database_shutdown_driver(Driver);

  /* Read points */
  npoints = Vect_get_num_primitives ( &Map, GV_POINT );
  z = (double *) G_malloc ( npoints * sizeof(double) );

  G_get_window (&window);   
  Vect_region_box ( &window, &box );

  Points = Vect_new_line_struct ();
  Cats = Vect_new_cats_struct ();

  nlines = Vect_get_num_lines ( &Map );
  nsites = 0;
  for( line = 1; line <= nlines ; line++) {
      int type, cat, ret, cval;
      double dval;

      G_debug ( 3, "line = %d", line );

      type = Vect_read_line ( &Map, Points, Cats, line );
      if ( !(type & GV_POINT) ) continue;

      if ( !all ) {
	 if ( !Vect_point_in_box ( Points->x[0], Points->y[0], 0.0, &box) ) continue; 
      }

      Vect_cat_get ( Cats, 1, &cat );
      
      G_debug ( 3, "cat = %d", cat );

      /* find actual value */
      if ( ctype == DB_C_TYPE_INT ) {
	  ret = db_CatValArray_get_value_int ( &cvarr, cat, &cval );
	  if ( ret != DB_OK ) {
	      G_warning ("No record for cat = %d", cat );
	      continue;
	  } 
	  dval = cval;
      } else if ( ctype == DB_C_TYPE_DOUBLE ) {
	  ret = db_CatValArray_get_value_double ( &cvarr, cat, &dval );
	  if ( ret != DB_OK ) {
	      G_warning ("No record for cat = %d", cat );
	      continue;
	  }
      }
      
      G_debug ( 3, "dval = %e", dval );
      z[nsites] = dval;
      nsites++;
  } 

  if (verbose)
    fprintf (stdout,"Number of points: %d\n", nsites);

  if (nsites <= 0)
    G_fatal_error ("No points found");

  if (nsites < 4)
    fprintf (stdout,"WARNING: I'm not so sure of myself for small samples\n");

  if (flag.l->answer)
  {
    G_message (_( "Doing log transformation"));
    warn_once = 0;
    for (i = 0; i < nsites; ++i)
    {
      if (z[i] > 1.0e-10)
	z[i] = log10 (z[i]);
      else if (!warn_once)
      {
	G_warning ("Negative or very small point values set to -10.0");
	z[i] = -10.0;
	warn_once = 1;
      }
    }
  }

  for (i = 0; parm.tests->answers[i]; i++)
    if (!scan_cats (parm.tests->answers[i], &x, &y))
    {
      G_usage ();
      exit (EXIT_FAILURE);
    }
  for (i = 0; parm.tests->answers[i]; i++)
  {
    scan_cats (parm.tests->answers[i], &x, &y);
    while (x <= y)
      switch (x++)
      {
      case 1:			/* moments */
	fprintf (stdout, "Moments \\sqrt{b_1} and b_2:");
	w = omnibus_moments (z, nsites);
	fprintf (stdout,"%g %g\n", w[0], w[1]);
	break;
      case 2:			/* geary */
	fprintf (stdout, "Geary's a-statistic & an approx. normal: ");
	w = geary_test (z, nsites);
	fprintf (stdout,"%g %g\n", w[0], w[1]);
	break;
      case 3:			/* extreme deviates */
	fprintf (stdout, "Extreme normal deviates: ");
	w = extreme (z, nsites);
	fprintf (stdout,"%g %g\n", w[0], w[1]);
	break;
      case 4:			/* D'Agostino */
	fprintf (stdout, "D'Agostino's D & an approx. normal: ");
	w = dagostino_d (z, nsites);
	fprintf (stdout,"%g %g\n", w[0], w[1]);
	break;
      case 5:			/* Kuiper */
	fprintf (stdout, "Kuiper's V (regular & modified for normality): ");
	w = kuipers_v (z, nsites);
	fprintf (stdout,"%g %g\n", w[1], w[0]);
	break;
      case 6:			/* Watson */
	fprintf (stdout,
		 "Watson's U^2 (regular & modified for normality): ");
	w = watson_u2 (z, nsites);
	fprintf (stdout,"%g %g\n", w[1], w[0]);
	break;
      case 7:			/* Durbin */
	fprintf (stdout,
		 "Durbin's Exact Test (modified Kolmogorov): ");
	w = durbins_exact (z, nsites);
	fprintf (stdout,"%g\n", w[0]);
	break;
      case 8:			/* Anderson-Darling */
	fprintf (stdout,
	  "Anderson-Darling's A^2 (regular & modified for normality): ");
	w = anderson_darling (z, nsites);
	fprintf (stdout,"%g %g\n", w[1], w[0]);
	break;
      case 9:			/* Cramer-Von Mises */
	fprintf (stdout,
	     "Cramer-Von Mises W^2(regular & modified for normality): ");
	w = cramer_von_mises (z, nsites);
	fprintf (stdout,"%g %g\n", w[1], w[0]);
	break;
      case 10:			/* Kolmogorov-Smirnov */
	fprintf (stdout,
	  "Kolmogorov-Smirnov's D (regular & modified for normality): ");
	w = kolmogorov_smirnov (z, nsites);
	fprintf (stdout,"%g %g\n", w[1], w[0]);
	break;
      case 11:			/* chi-square */
	fprintf (stdout,
	       "Chi-Square stat (equal probability classes) and d.f.: ");
	w = chi_square (z, nsites);
	fprintf (stdout,"%g %d\n", w[0], (int) w[1]);
	break;
      case 12:			/* Shapiro-Wilk */
	if (nsites > 50)
	{
	  fprintf (stdout, "\nShapiro-Wilk's W cannot be used for n > 50\n");
	  if (nsites < 99)
	    fprintf (stdout, "Use Weisberg-Binghams's W''\n\n");
	  else
	    fprintf (stdout, "\n");
	}
	else
	{
	  fprintf (stdout, "Shapiro-Wilk W: ");
	  w = shapiro_wilk (z, nsites);
	  fprintf (stdout,"%g\n", w[0]);
	}
	break;
      case 13:			/* Weisberg-Bingham */
	if (nsites > 99 || nsites < 50)
	  fprintf (stdout,
		   "\nWeisberg-Bingham's W'' cannot be used for n < 50 or n > 99\n\n");
	else
	{
	  fprintf (stdout, "Weisberg-Bingham's W'': ");
	  w = weisberg_bingham (z, nsites);
	  fprintf (stdout,"%g\n", w[0]);
	}
	break;
      case 14:			/* Royston */
	if (nsites > 2000)
	  fprintf (stdout,
	  "\nRoyston only extended Shapiro-Wilk's W up to n = 2000\n\n");
	else
	{
	  fprintf (stdout, "Shapiro-Wilk W'': ");
	  w = royston (z, nsites);
	  fprintf (stdout,"%g\n", w[0]);
	}
	break;
      case 15:			/* Kotz */
	fprintf (stdout,
		 "Kotz' T'_f (Lognormality vs. Normality): ");
	w = kotz_families (z, nsites);
	fprintf (stdout,"%g\n", w[0]);
	break;
      default:
	break;
      }
  }
  exit (EXIT_SUCCESS);
}
