/*!
  \file lib/vector/vedit/delete.c

  \brief Vedit library - delete primitives

  (C) 2007-2008 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vedit.h>

/*!
  \brief Delete selected primitives
  
  \param Map pointer to Map_info
  \param List list of primitives to be deleted
  
  \return number of deleted primitives
  \return -1 on on error
*/
int Vedit_delete_lines(struct Map_info *Map, struct ilist *List)
{
    int i, line;
    int nlines_removed;

    nlines_removed = 0;

    /* delete */
    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];

	if (Vect_line_alive(Map, line)) {
	    if (Vect_delete_line(Map, line) < 0) {
		return -1;
	    }

	    G_debug(3, "Vedit_delete_lines(): line=%d", line);
	    nlines_removed++;
	}
    }

    return nlines_removed;
}
