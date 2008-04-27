/* $Id$
 **************************************************************
 *
 * MODULE:       v.in.region
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Create a new vector from current region 
 *               
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    int    type, cat;
    struct Option *out_opt, *type_opt, *cat_opt;
    struct GModule *module;
    struct Map_info Out;
    struct Cell_head window;
    struct line_cats *Cats; 
    struct line_pnts *Points;
    double diff_long, mid_long;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector");
    module->description =
	_("Create a new vector from the current region.");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt->multiple = NO;
    type_opt->options = "line,area";
    type_opt->answer = "area";
    type_opt->description  = _("Select type: line or area");

    cat_opt = G_define_standard_option(G_OPT_V_CAT);
    cat_opt->answer = "1";

    if(G_parser(argc,argv))
	exit(EXIT_FAILURE);

    Cats = Vect_new_cats_struct ();
    Points = Vect_new_line_struct ();

    type = Vect_option_to_types ( type_opt );
    cat = atoi( cat_opt->answer );

    G_get_window (&window);
    diff_long = window.east - window.west;
    mid_long = (window.west + window.east) / 2;

    /* Open output segments */
    Vect_open_new ( &Out, out_opt->answer, 0 );
    Vect_hist_command ( &Out );

    /* Rectangle */

    Vect_append_point ( Points, window.west, window.south, 0.0 );
    if (window.proj == PROJECTION_LL && diff_long >= 179)
	Vect_append_point ( Points, mid_long, window.south, 0.0 );
    Vect_append_point ( Points, window.east, window.south, 0.0 );
    Vect_append_point ( Points, window.east, window.north, 0.0 );
    if (window.proj == PROJECTION_LL && diff_long >= 179)
	Vect_append_point ( Points, mid_long, window.north, 0.0 );
    Vect_append_point ( Points, window.west, window.north, 0.0 );
    Vect_append_point ( Points, window.west, window.south, 0.0 );


    if ( type == GV_AREA ) {
	Vect_write_line ( &Out, GV_BOUNDARY, Points, Cats );

	Vect_reset_line ( Points );
	Vect_append_point ( Points, (window.west+window.east)/2, (window.south+window.north)/2, 0.0 );

	Vect_cat_set (Cats, 1, cat);
	Vect_write_line ( &Out, GV_CENTROID, Points, Cats );
    } else { /* GV_LINE */
	Vect_cat_set (Cats, 1, cat);
	Vect_write_line ( &Out, GV_LINE, Points, Cats );
    }

    Vect_build (&Out, stderr);
    Vect_close (&Out);

    exit(EXIT_SUCCESS);
}
