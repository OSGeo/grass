/***************************************************************
 *
 * MODULE:       v.split
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Split lines to segments
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h> 
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int 
main (int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_opt, *out_opt, *length_opt, *vertices_opt;
    /* struct Option *layer_opt; */
    struct Map_info In, Out;
    struct line_pnts *Points, *Points2;
    struct line_cats *Cats;
    /* int    layer; */
    int    line, nlines;
    double length;
    int    vertices;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, geometry");
    module->description = "Split lines to shorter segments.";

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    /* layer_opt = G_define_standard_option(G_OPT_V_FIELD); */
    
    length_opt = G_define_option();
    length_opt->key = "length";
    length_opt->type =  TYPE_DOUBLE;
    length_opt->required = NO;
    length_opt->multiple = NO;
    length_opt->description = "Maximum segment length.";

    vertices_opt = G_define_option();
    vertices_opt->key = "vertices";
    vertices_opt->type =  TYPE_INTEGER;
    vertices_opt->required = NO;
    vertices_opt->multiple = NO;
    vertices_opt->description = "Maximum number of vertices in segment.";

    if (G_parser (argc, argv)) exit(-1); 

    /* layer = atoi ( layer_opt->answer ); */

    if ( (length_opt->answer && vertices_opt->answer) ||
	!(length_opt->answer || vertices_opt->answer) ) 
	G_fatal_error ("Use either length or vertices" );
    
    if ( length_opt->answer )
        length = atof ( length_opt->answer );

    if ( vertices_opt->answer ) {
        vertices = atoi ( vertices_opt->answer );
	if ( vertices < 2 ) 
	    G_fatal_error ( "Number of vertices must be at least 2" );
    }

    Vect_set_open_level (2); 
    Vect_open_old (&In, in_opt->answer, ""); 
    Vect_open_new (&Out, out_opt->answer, 0);
    Vect_copy_head_data (&In, &Out);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command ( &Out );
    
    Points = Vect_new_line_struct ();
    Points2 = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();

    nlines = Vect_get_num_lines ( &In );

    for ( line = 1; line <= nlines; line++ ) {
	int ltype;
	
	G_percent ( line, nlines, 1 );

	ltype = Vect_read_line ( &In, Points, Cats, line);
	
	if ( ltype & GV_LINES ) {
	    if ( length_opt->answer ) { 
	        double l, from, to, step;

	        l = Vect_line_length (Points);	

		if ( l <= length ) {
	            Vect_write_line ( &Out, ltype, Points, Cats );
		} else {
		    int n, i;
		
		    n = ceil ( l / length );
		    step = l / n;
		    from = 0.;

		    for ( i = 0; i< n; i++ ) {
			int ret ;
			double x, y;

			if ( i == n-1 ) {
			    to = l; /* to be sure that it goes to end */
			} else {
			    to = from+step;
			}
			
		        ret = Vect_line_segment ( Points, from, to, Points2 );
			if ( ret == 0 ) {
			    G_warning ( "Cannot make line segment: %f - %f (line length = %f)",
				          from, to, l );
			    continue;
			}
		    
			/* To be sure that the coordinates are identical */
			if ( i > 0 ) {
			    Points2->x[0] = x;
			    Points2->y[0] = y;
			}
			if ( i == n-1 ) {
			    Points2->x[Points2->n_points-1] = Points->x[Points->n_points-1];
			    Points2->y[Points2->n_points-1] = Points->y[Points->n_points-1];
			}

	                Vect_write_line ( &Out, ltype, Points2, Cats );
			
			/* last point */
			x = Points2->x[Points2->n_points-1];
		        y = Points2->y[Points2->n_points-1];	
			
			from += step;
		    }
		}
	    } else {
		int start = 0; /* number of coordinates written */
		while ( start < Points->n_points - 1 ) {
		    int i, v;

		    Vect_reset_line ( Points2 );
		    for ( i = 0; i < vertices; i++ ) {
			v = start + i; 
			if ( v == Points->n_points ) break;
		    
			Vect_append_point ( Points2, Points->x[v], Points->y[v], Points->z[v] );
		    }

	            Vect_write_line ( &Out, ltype, Points2, Cats );

		    start = v;
		}
	    }
	} else {
	    Vect_write_line ( &Out, ltype, Points, Cats );
	}
    }
    
    Vect_close (&In);
    Vect_build (&Out, stderr);
    Vect_close (&Out);

    exit(0) ;
}

