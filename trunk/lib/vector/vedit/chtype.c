/*!
  \file vector/vedit/chtype.c
  
  \brief Vedit library - change primitives type
  
  (C) 2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vedit.h>

/*!
  \brief Change primitive type
  
  Supported conversions:
   - point <-> centroid
   - line <-> boundary

   \param Map pointer to Map_info
   \param List list of selected primitives
   
   \return number of modified primitives
   \return -1 on error
*/
int Vedit_chtype_lines(struct Map_info *Map, struct ilist *List)
{
    int i;
    int nret, line;
    int type, newtype;
    struct line_pnts *Points;
    struct line_cats *Cats;

    nret = 0;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];
	if (!Vect_line_alive(Map, line))
	    continue;
	type = Vect_read_line(Map, Points, Cats, line);
	if (type < 0) {
	    return -1;
	}
	switch (type) {
	case GV_POINT:
	    newtype = GV_CENTROID;
	    break;
	case GV_CENTROID:
	    newtype = GV_POINT;
	    break;
	case GV_LINE:
	    newtype = GV_BOUNDARY;
	    break;
	case GV_BOUNDARY:
	    newtype = GV_LINE;
	    break;
	default:
	    newtype = -1;
	    break;
	}

	G_debug(3, "Vedit_chtype_lines(): line=%d, from_type=%d, to_type=%d",
		line, type, newtype);

	if (newtype > 0) {
	    if (Vect_rewrite_line(Map, line, newtype, Points, Cats) < 0) {
		return -1;
	    }
	    nret++;
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nret;
}
