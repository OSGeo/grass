/* ***************************************************************
 * *
 * * MODULE:       v.clean
 * * 
 * * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 * *               
 * * PURPOSE:      Clean lines -- remove all lines or boundaries
 * *               of zero length
 * *               
 * * COPYRIGHT:    (C) 2007 by the GRASS Development Team
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

int remove_zero_line(struct Map_info *Map, int otype, struct Map_info *Err)
{
    int count;
    int line, type, nlines;

    struct line_pnts *Points;
    struct line_cats *Cats;

    count = 0;
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);

    G_debug(1, "nlines =  %d", nlines);

    for (line = 1; line <= nlines; line++) {
	G_percent(line, nlines, 2);
	if (!Vect_line_alive(Map, line))
	    continue;

	type = Vect_read_line(Map, Points, Cats, line);

	if (!((type & GV_LINES) && (type & otype)))
	    continue;

	if (Vect_line_prune(Points) > 1)
	    continue;

	Vect_delete_line(Map, line);

	if (Err) {
	    Vect_write_line(Err, type, Points, Cats);
	}

	count++;
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    G_verbose_message(_("Lines / boundaries removed: %d"), count);

    return count;

}
