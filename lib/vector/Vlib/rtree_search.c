/*!
   \file lib/vector/Vlib/rtree_search.c

   \brief Vector library - simplified rtree search

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Soeren Gebbert
 */

#include <assert.h>
#include <grass/vector.h>

/* Function to add the ids of overlapping rectangles to an ilist
 * This function is a callback function used in RTreeSearch2()
 * */
static int add_id_to_list(int id, const struct RTree_Rect *rect, void *list)
{
    struct ilist *l = (struct ilist*)list;
    G_ilist_add(l, id);
    return 1;
}

/**
 * Search in an index tree for all data retangles that
 * overlap the argument rectangle.
 *
 * \param t: The RTree 
 * \param r: The argument rectangle
 * \param list: The list to store the ids of overlapping rectangles
 * \return the number of qualifying data rects.
 */
int RTreeSearch2(struct RTree *t, struct RTree_Rect *r, struct ilist *list)
{
    assert(r && t);

    G_init_ilist(list);

    return t->search_rect(t, r, add_id_to_list, (void*)list);
}

