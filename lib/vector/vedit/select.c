/*!
  \file lib/vector/vedit/select.c

  \brief Vedit library - select primitives by query
  
  (C) 2007-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <math.h>
#include <grass/glocale.h>
#include <grass/vedit.h>

static int select_by_query(struct Map_info *, int, int, double,
			   int, struct line_pnts *, struct line_cats *);

static int merge_lists(struct ilist *alist, struct ilist *blist);

/*!
  \brief Select primitives by query (based on geometry properties)
  
  Currently supported:
   - QUERY_LENGTH, select all lines longer than threshold (or shorter if threshold is < 0)
   - QUERY_DANGLE, select all dangles longer than threshold (or shorter if threshold is < 0)

  Perform global query if <i>List</i> is empty otherwise query only
  selected vector objects.

  \param Map pointer to Map_info
  \param type feature type
  \param layer layer number
  \param thresh threshold value (< 0 for 'shorter', > 0 for 'longer')
  \param query query (length, dangle, ...)
  \param[in,out] List list of selected features
  
  \return number of selected primitives
*/
int Vedit_select_by_query(struct Map_info *Map,
			  int type, int layer, double thresh, int query,
			  struct ilist *List)
{
    int num, line, i;
    double thresh_tmp;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct ilist *List_query;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if (List->n_values == 0) {
	List_query = List;
    }
    else {
	List_query = Vect_new_list();
    }

    switch (query) {
    case QUERY_LENGTH:{
	    if (List->n_values == 0) {
		/* query all vector objects in vector map */
		num = Vect_get_num_lines(Map);
		for (line = 1; line <= num; line++) {
		    if (select_by_query(Map, line, type, thresh,
					query, Points, Cats))
			Vect_list_append(List_query, line);
		}
	    }
	    else {
		for (i = 0; i < List->n_values; i++) {
		    line = List->value[i];
		    if (select_by_query(Map, line, type, thresh,
					query, Points, Cats)) {
			Vect_list_append(List_query, line);
		    }
		}
	    }
	    break;
	}
    case QUERY_DANGLE:{
	    struct ilist *List_dangle;

	    List_dangle = Vect_new_list();
	    thresh_tmp = fabs(thresh);

	    /* select dangles shorter than 'thresh_tmp' */
	    Vect_select_dangles(Map, type, thresh_tmp, List_dangle);

	    if (thresh <= 0.0) {	/* shorter than */
		for (i = 0; i < List_dangle->n_values; i++) {
		    Vect_list_append(List_query, List_dangle->value[i]);
		}
	    }
	    else {		/* longer than */
		for (i = 1; i <= Vect_get_num_lines(Map); i++) {
		    if (!Vect_val_in_list(List_dangle, i))
			Vect_list_append(List_query, i);
		}
	    }

	    Vect_destroy_list(List_dangle);
	    break;
	}
    default:
	break;
    }

    if (List != List_query) {
	merge_lists(List, List_query);
	Vect_destroy_list(List_query);
    }

    G_debug(3, "Vedit_select_by_query(): %d lines selected (by query %d)",
	    List->n_values, query);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return List->n_values;
}

/*!
   \brief Query selected primitive

   \return 1 line test positive 
   \return 0 line test negative
   \return -1 on error (line is dead)
*/
int select_by_query(struct Map_info *Map, int line, int type, double thresh,
		    int query, struct line_pnts *Points,
		    struct line_cats *Cats)
{
    int ltype;
    double length;
    int i, cat_curr;
    int node1, node2, node;	/* nodes */
    int nnode1, nnode2;	/* number of line in node */
    double nx, ny, nz;	/* node coordinates */
    struct ilist *exclude, *found;	/* line id of nearest lines */
    struct line_cats *Cats_curr;

    if (!Vect_line_alive(Map, line))
	return -1;

    ltype = Vect_read_line(Map, Points, Cats, line);

    if (!(ltype & type))
	return -1;

    if (query == QUERY_LENGTH) {
	length = Vect_line_length(Points);
	if (thresh <= 0.0) {	/* shorter then */
	    if (length <= fabs(thresh))
		return 1;
	}
	else {			/* longer then */
	    if (length > thresh)
		return 1;
	}
    }
    else if (query == QUERY_DANGLE) {
	/*
	   this code is currently replaced by Vect_select_dangle()
	   not used by v.edit
	 */
	int layer, cat;

	layer = 1;
	Vect_cat_get(Cats, layer, &cat);	/* get first category from layer */
	if (!(type & GV_LINES))
	    return -1;
	/* check if line is dangle */

	Vect_get_line_nodes(Map, line, &node1, &node2);

	node = -1;
	nnode1 = Vect_get_node_n_lines(Map, node1);
	nnode2 = Vect_get_node_n_lines(Map, node2);

	if ((nnode1 == 4 && nnode2 == 1) || (nnode1 == 1 && nnode2 == 4)) {
	    if (nnode1 == 4)
		node = node1;
	    else
		node = node2;
	}

	/* no dangle ? */
	if (node == -1)
	    return -1;

	length = Vect_line_length(Points);
	if (thresh <= 0.0) {	/* shorter then */
	    if (length > fabs(thresh))
		return -1;
	}
	else {			/* longer then */
	    if (length <= thresh)
		return -1;
	}

	/* at least one of the lines need to have same category number */
	exclude = Vect_new_list();
	found = Vect_new_list();

	Vect_get_node_coor(Map, node, &nx, &ny, &nz);

	Vect_list_append(exclude, line);
	Vect_find_line_list(Map, nx, ny, nz,
			    GV_LINES, 0.0, WITHOUT_Z, exclude, found);

	Cats_curr = Vect_new_cats_struct();

	for (i = 0; i < found->n_values; i++) {
	    Vect_read_line(Map, NULL, Cats_curr, found->value[i]);
	    if (Vect_cat_get(Cats_curr, layer, &cat_curr) > -1) {
		if (cat == cat_curr)
		    return 1;
	    }
	}

	Vect_destroy_cats_struct(Cats_curr);
	Vect_destroy_list(exclude);
	Vect_destroy_list(found);
    }
    else {
	/* this shouldn't happen */
	G_fatal_error("Vedit_select_by_query(): %s", _("Unknown query tool"));
    }

    return 0;
}

/*!
   \brief Merge two lists, i.e. store only duplicate items

   \param[in,out] alist list to be merged
   \param blist list used for merging

   \return result number of items
*/
int merge_lists(struct ilist *alist, struct ilist *blist)
{
    int i;

    struct ilist *list_del;

    list_del = Vect_new_list();

    for (i = 0; i < alist->n_values; i++) {
	if (!Vect_val_in_list(blist, alist->value[i]))
	    Vect_list_append(list_del, alist->value[i]);
    }

    Vect_list_delete_list(alist, list_del);

    Vect_destroy_list(list_del);

    return alist->n_values;
}
