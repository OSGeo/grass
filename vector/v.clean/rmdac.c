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
#include <grass/vector.h>
#include <grass/glocale.h>

int rmdac(struct Map_info *Out, struct Map_info *Err)
{
    int i, type, area, ndupl, nlines;

    struct line_pnts *Points;
    struct line_cats *Cats;

    nlines = Vect_get_num_lines(Out);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    G_debug(1, "nlines =  %d", nlines);

    ndupl = 0;

    for (i = 1; i <= nlines; i++) {
	G_percent(i, nlines, 2);
	if (!Vect_line_alive(Out, i))
	    continue;

	type = Vect_read_line(Out, Points, Cats, i);
	if (!(type & GV_CENTROID))
	    continue;

	area = Vect_get_centroid_area(Out, i);
	G_debug(3, "  area = %d", area);

	if (area < 0) {
	    Vect_delete_line(Out, i);
	    ndupl++;

	    if (Err) {
		Vect_write_line(Err, type, Points, Cats);
	    }
	}
    }

    G_verbose_message(_("Duplicate area centroids: %d"), ndupl);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return ndupl;
}
