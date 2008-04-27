/* ***************************************************************
 * *
 * * MODULE:       v.clean
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Clean lines - remove duplicate centroids
 * *               
 * * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdlib.h> 
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int 
rmdac ( struct Map_info *Out, struct Map_info *Err )
{
        int    i, type, area, ndupl, nlines;

	struct line_pnts *Points;
	struct line_cats *Cats;

	nlines = Vect_get_num_lines (Out);

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

        G_debug (1, "nlines =  %d", nlines );

	ndupl = 0;
	if (G_verbose() > G_verbose_min())
	  fprintf (stderr, _("Duplicate area centroids: %5d"), ndupl);

	for ( i = 1; i <= nlines; i++ ){ 
	    if ( !Vect_line_alive ( Out, i ) ) continue;

	    type = Vect_read_line (Out, Points, Cats, i);
	    if ( !(type & GV_CENTROID) ) continue;

	    area = Vect_get_centroid_area ( Out, i );
            G_debug (3, "  area = %d", area);
	    
	    if ( area < 0 ) {
		Vect_delete_line (Out, i); 
		ndupl++;

		if (G_verbose() > G_verbose_min())
		  fprintf (stderr, "\r%s: %5d", _("Duplicate area centroids"), ndupl);

		if (Err) {
		    Vect_write_line(Err, type, Points, Cats);
		}
	    }
	}

	if (G_verbose() > G_verbose_min())
	  fprintf (stderr, "\n");
	
	Vect_destroy_line_struct(Points);
	Vect_destroy_cats_struct(Cats);

	return ndupl;
}


