/*!
  \file lib/vector/vedit/flip.c
   
  \brief Vedit library - flip lines
  
  (C) 2007-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vedit.h>

/*!
  \brief Flip direction of selected lines
  
  \param Map pointer to Map_info
  \param List list of selected lines
  
  \return number of modified lines
  \return -1 on error
*/
int Vedit_flip_lines(struct Map_info *Map, struct ilist *List)
{
    struct line_cats *Cats;
    struct line_pnts *Points;
    int i, line, type;
    int nlines_flipped;

    nlines_flipped = 0;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];

	if (!Vect_line_alive(Map, line))
	    continue;

	type = Vect_read_line(Map, Points, Cats, line);

	if (!(type & GV_LINES))
	    continue;

	Vect_line_reverse(Points);

	if (Vect_rewrite_line(Map, line, type, Points, Cats) < 0) {
	    return -1;
	}

	G_debug(3, "Vedit_flip_lines(): line=%d", line);

	nlines_flipped++;
    }

    /* destroy structures */
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nlines_flipped;
}
