/*!
   \file lib/vector/Vlib/select.c

   \brief Vector library - spatial index

   Higher level functions for a custom spatial index.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Initialize spatial index structure

   \param si pointer to spatial index structure

   \return void
 */
void Vect_spatial_index_init(struct spatial_index * si, int with_z)
{
    G_debug(1, "Vect_spatial_index_init()");

    si->si_tree = RTreeCreateTree(-1, 0, 2 + (with_z != 0));
}

/*!
   \brief Destroy existing spatial index

   Vect_spatial_index_init() must be call before new use.

   \param si pointer to spatial index structure

   \return void
 */
void Vect_spatial_index_destroy(struct spatial_index * si)
{
    G_debug(1, "Vect_spatial_index_destroy()");

    RTreeDestroyTree(si->si_tree);
}

/*!
   \brief Add a new item to spatial index structure

   \param[in,out] si pointer to spatial index structure
   \param id item identifier
   \param box pointer to item bounding box

   \return void
 */
void Vect_spatial_index_add_item(struct spatial_index * si, int id,
				 const struct bound_box * box)
{
    static struct RTree_Rect rect;
    static int rect_init = 0;

    if (!rect_init) {
	rect.boundary = G_malloc(si->si_tree->nsides_alloc * sizeof(RectReal));
	rect_init = si->si_tree->nsides_alloc;
    }

    G_debug(3, "Vect_spatial_index_add_item(): id = %d", id);

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;
    RTreeInsertRect(&rect, id, si->si_tree);
}

/*!
   \brief Delete item from spatial index structure

   \param[in,out] si pointer to spatial index structure
   \param id item identifier

   \return void
 */
void Vect_spatial_index_del_item(struct spatial_index * si, int id,
				 const struct bound_box * box)
{
    int ret;
    static struct RTree_Rect rect;
    static int rect_init = 0;

    if (!rect_init) {
	rect.boundary = G_malloc(si->si_tree->nsides_alloc * sizeof(RectReal));
	rect_init = si->si_tree->nsides_alloc;
    }

    G_debug(3, "Vect_spatial_index_del_item(): id = %d", id);

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;

    ret = RTreeDeleteRect(&rect, id, si->si_tree);

    if (ret)
	G_fatal_error(_("Unable to delete item %d from spatial index"), id);
}

/************************* SELECT BY BOX *********************************/
/* This function is called by  RTreeSearch() to add selected item to the list */
static int _add_item(int id, const struct RTree_Rect *rect, struct ilist *list)
{
    G_ilist_add(list, id);
    return 1;
}

/*!
   \brief Select items by bounding box to list

   \param si pointer to spatial index structure
   \param box bounding box
   \param[out] list pointer to list where selected items are stored

   \return number of selected items
 */
int Vect_spatial_index_select(const struct spatial_index * si, const struct bound_box * box,
                              struct ilist *list)
{
    static struct RTree_Rect rect;
    static int rect_init = 0;

    if (!rect_init) {
	rect.boundary = G_malloc(si->si_tree->nsides_alloc * sizeof(RectReal));
	rect_init = si->si_tree->nsides_alloc;
    }

    Vect_reset_list(list);

    rect.boundary[0] = box->W;
    rect.boundary[1] = box->S;
    rect.boundary[2] = box->B;
    rect.boundary[3] = box->E;
    rect.boundary[4] = box->N;
    rect.boundary[5] = box->T;
    RTreeSearch(si->si_tree, &rect, (void *)_add_item, list);

    G_debug(3, "Vect_spatial_index_select(): %d items selected", list->n_values);

    return list->n_values;
}
