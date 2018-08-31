/*!
  \file lib/vector/vedit/cats.c

  \brief Vedit library - category manipulation
  
  (C) 2006-2008 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Jachym Cepicky <jachym.cepicky gmail.com>
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/glocale.h>
#include <grass/vedit.h>

/*!
  \brief Add / remove categories
  
  \param Map pointer to Map_info
  \param List list of selected primitives
  \param layer layer number
  \param del action (non-zero for delete otherwise add)
  \param Clist list of category numbers
  
  \return number of modified primitives
  \return -1 on error
*/
int Vedit_modify_cats(struct Map_info *Map, struct ilist *List,
		      int layer, int del, struct cat_list *Clist)
{
    int i, j;
    struct line_cats *Cats;
    struct line_pnts *Points;
    int line, type, cat;
    int nlines_modified, rewrite;

    /* features defined by cats */
    if (Clist->n_ranges <= 0) {
	return 0;
    }

    nlines_modified = 0;

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();

    /* for each line, set new category */
    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];
	type = Vect_read_line(Map, Points, Cats, line);

	if (!Vect_line_alive(Map, line))
	    continue;

	rewrite = 0;
	for (j = 0; j < Clist->n_ranges; j++) {
	    for (cat = Clist->min[j]; cat <= Clist->max[j]; cat++) {
		/* add new category */
		if (!del) {
		    if (Vect_cat_set(Cats, layer, cat) < 1) {
			G_warning(_("Unable to set category %d for (feature id %d)"),
				  cat, line);
		    }
		    else {
			rewrite = 1;
		    }
		}
		else {		/* delete old category */
		    if (Vect_field_cat_del(Cats, layer, cat) > 0) {
			rewrite = 1;
		    }
		}
	    }
	}

	if (rewrite == 0)
	    continue;

	if (Vect_rewrite_line(Map, line, type, Points, Cats) < 0) {
	    return -1;
	}

	nlines_modified++;

    }

    /* destroy structures */
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nlines_modified;
}
