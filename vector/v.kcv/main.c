/*
 * from s.kcv
 * Copyright (C) 1993-1994. James Darrell McCauley.
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
 * 4.2B <27 Jan 1994>  fixed RAND_MAX for Solaris 2.3
 * <13 Sep 2000> released under GPL
 * 10/2004 Upgrade to 5.7 Radim Blazek
 */
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "kcv.h"

#ifndef RAND_MAX 
#define RAND_MAX (pow(2.0,31.0)-1) 
#endif
#if defined(__CYGWIN__) || defined(__APPLE__) || defined(__MINGW32__)
double drand48()
{
	return(rand()/32767.0);
}
#define srand48(sv) (srand((unsigned)(sv)))
#else
double drand48 ();
void srand48 ();
#endif

struct Cell_head window;

int 
main (int argc, char *argv[])
{
    int    line, nlines;
    double east, north, (*rng) (), max, myrand ();
    int    i, j, n, nsites, verbose, np, *p, dcmp ();
    int    *pnt_part;
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    char   *mapset;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *col_opt, *npart_opt;
    struct Flag *drand48_flag, *q_flag;
    BOUND_BOX box;
    double maxdist;
  
    /* Attributes */
    struct field_info *Fi;
    dbDriver *Driver;
    char buf[2000];
    dbString sql;

    module = G_define_module();
    module->keywords = _("vector, statistics");
    module->description = _("Randomly partition points into test/train sets.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
  
    npart_opt = G_define_option ();
    npart_opt->key = "k";
    npart_opt->type = TYPE_INTEGER;
    npart_opt->required = YES;
    npart_opt->description = _("Number of partitions");
    npart_opt->options = "1-32767";
    
    col_opt = G_define_option();
    col_opt->key = "column";
    col_opt->type =  TYPE_STRING;
    col_opt->required = YES;
    col_opt->multiple = NO;
    col_opt->answer = "part";
    col_opt->description = _("Name for new column to which partition number is written");

    drand48_flag = G_define_flag ();
    drand48_flag->key = 'd';
    drand48_flag->description = _("Use drand48()");

    /* please remove in GRASS 7 */
    q_flag = G_define_flag ();
    q_flag->key = 'q';
    q_flag->description = _("Quiet");

    G_gisinit(argv[0]);

    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    verbose = (!q_flag->answer);
    np = atoi ( npart_opt->answer);

    if ( drand48_flag->answer )
    {
      rng = drand48;
      max = 1.0;
      srand48 ((long) getpid ());
    }
    else
    {
      rng = myrand;
      max = RAND_MAX;
      srand (getpid ());
    }
    
    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    
    /* open input vector */
    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) {
	 G_fatal_error (_("Vector map <%s> not found"), in_opt->answer);
    }
    
    Vect_set_open_level (2);
    if (Vect_open_old (&In, in_opt->answer, mapset) < 2) {
	G_fatal_error (_("Unable to open vector map <%s> at topological level %d"),
		       in_opt->answer, 2);
    }

    Vect_get_map_box ( &In, &box );
  
    nsites = Vect_get_num_primitives(&In, GV_POINT);

    if ( nsites < np ) {
        G_fatal_error ( "Sites found: %i\nMore partitions than sites.", nsites );
    }

    Vect_set_fatal_error (GV_FATAL_PRINT);
    Vect_open_new (&Out, out_opt->answer, Vect_is_3d(&In) );
	
    Vect_copy_head_data (&In, &Out);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command ( &Out );

    /* Copy vector lines */
    Vect_copy_map_lines ( &In, &Out );

    /* Copy tables */
    Vect_copy_tables ( &In, &Out, 0 );

    /* Add column */
    db_init_string (&sql);

    Fi = Vect_get_field( &Out, 1);
    if ( Fi == NULL ) {
	G_fatal_error (_("Unable to get layer info for vector map <%s>"),
		       in_opt -> answer);
    }

    Driver = db_start_driver_open_database ( Fi->driver, Fi->database );
    if (Driver == NULL)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			Fi->database, Fi->driver);

    sprintf ( buf, "alter table %s add column %s integer", Fi->table, col_opt->answer );

    db_set_string ( &sql, buf);

    G_debug ( 3, "SQL: %s", db_get_string ( &sql) );
    
    if (db_execute_immediate (Driver, &sql) != DB_OK ) {
      G_fatal_error (_("Cannot alter table: %s"), db_get_string ( &sql ) );
    }

    /*
     * make histogram of number sites in each test partition since the
     * number of sites will not always be a multiple of the number of
     * partitions. make_histo() returns the maximum bin size.
     */
    n = make_histo (&p, np, nsites);

    nlines = Vect_get_num_lines ( &In );
    pnt_part = (int *) G_calloc ( nlines+1, sizeof(int) );

    maxdist = 1.1 * sqrt ( pow(box.E-box.W,2.0) + pow(box.N-box.S,2.0) );

    /* Assign fold to each point */
    for (i = 0; i < np; ++i) {	/* for each partition */
	for (j = 0; j < p[i]; ++j) {	/* create p[i] random points */
	  int nearest = 0;
	  double dist;

	  east = rng () / max * (box.E - box.W) + box.W;
	  north = rng () / max * (box.N - box.S) + box.S;
	      
	  G_debug ( 3, "east = %f north = %f", east, north );

	  /* find nearest */
	  for ( line = 1; line <= nlines; line++ ) {
	      int type;
	      double cur_dist;
	      
	      if ( pnt_part[line] > 0 ) continue;

	      type = Vect_read_line (&In, Points, Cats, line);

	      if ( !(type & GV_POINT) ) continue;
	      
	      cur_dist = hypot ( Points->x[0] - east, Points->y[0] - north);

	      if ( nearest < 1 || cur_dist < dist ) {
		nearest = line;
		dist = cur_dist;
	      }
	  }
	  
	  G_debug ( 3, "nearest = %d", nearest );

	  /* Update */
	  if ( nearest > 0 ) { /* shopuld be always */
	      int cat;

	      Vect_read_line (&In, Points, Cats, nearest);
	      Vect_cat_get ( Cats, 1, &cat );

	      sprintf ( buf, "update %s set %s = %d where %s = %d", Fi->table, col_opt->answer,
		                      i+1, Fi->key, cat );

	      db_set_string ( &sql, buf);

	      G_debug ( 3, "SQL: %s", db_get_string ( &sql ) );

	      if (db_execute_immediate (Driver, &sql) != DB_OK ) {
	          G_fatal_error (_("Unable to insert row: %s"), db_get_string ( &sql ) );
              }
	      pnt_part[nearest] = i+1;

	  }
	}
    }

    Vect_close (&In);
  
    db_close_database_shutdown_driver(Driver);

    Vect_build (&Out, stderr);
    Vect_close (&Out);

    exit(EXIT_SUCCESS);
}

