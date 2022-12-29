
/*!
 * \file lib/vector/Vlib/list.c
 *
 * \brief Vector library - list definition
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Original author CERL, probably Dave Gerdes or Mike Higgins.
 * \author Update to GRASS 5.7 Radim Blazek and David D. Gray
 * \author Update to GRASS 7 Markus Metz
 */

#include <stdlib.h>
#include <grass/vector.h>

/**
 * \brief Creates and initializes a struct ilist.
 *
 * This structure is used as container for integer values. The
 * library routines handle all memory allocation.
 *
 * \return pointer to struct ilist
 * \return NULL on error
 */
struct ilist *Vect_new_list(void)
{
    struct ilist *p;

    p = (struct ilist *)G_malloc(sizeof(struct ilist));

    if (p) {
	p->value = NULL;
	p->n_values = 0;
	p->alloc_values = 0;
    }

    return p;
}

/**
 * \brief Reset ilist structure.
 *
 * To make sure ilist structure is clean to be re-used. List must have
 * previously been created with Vect_new_list().
 *
 * \param[in,out] list pointer to struct ilist
 * 
 * \return 0
 */
int Vect_reset_list(struct ilist *list)
{
    list->n_values = 0;

    return 0;
}

/**
 * \brief Frees all memory associated with a struct ilist, including
 * the struct itself
 *
 * \param[in,out] list pointer to ilist structure
 */
void Vect_destroy_list(struct ilist *list)
{
    if (list) {			/* probably a moot test */
	if (list->alloc_values) {
	    G_free((void *)list->value);
	}
	G_free((void *)list);
    }
    list = NULL;
}

/**
 * \brief Append new item to the end of list if not yet present 
 *
 * \param[in,out] list pointer to ilist structure
 * \param val new item to append to the end of list
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_list_append(struct ilist *list, int val)
{
    int i;
    size_t size;

    if (list == NULL)
	return 1;

    for (i = 0; i < list->n_values; i++) {
	if (val == list->value[i])
	    return 0;
    }

    if (list->n_values == list->alloc_values) {
	size = (list->n_values + 1000) * sizeof(int);
	list->value = (int *)G_realloc((void *)list->value, size);
	list->alloc_values = list->n_values + 1000;
    }

    list->value[list->n_values] = val;
    list->n_values++;

    return 0;
}

/**
 * \brief Append new items to the end of list if not yet present 
 *
 * \param[in,out] alist pointer to ilist structure where items will be appended
 * \param blist pointer to ilist structure with new items
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_list_append_list(struct ilist *alist, const struct ilist *blist)
{
    int i;

    if (alist == NULL || blist == NULL)
	return 1;

    for (i = 0; i < blist->n_values; i++)
	Vect_list_append(alist, blist->value[i]);

    return 0;
}

/**
 * \brief Remove a given value (item) from list
 *
 * \param[in,out] list pointer to ilist structure
 * \param val to remove
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_list_delete(struct ilist *list, int val)
{
    int i, j;

    if (list == NULL)
	return 1;

    for (i = 0; i < list->n_values; i++) {
	if (val == list->value[i]) {
	    for (j = i + 1; j < list->n_values; j++)
		list->value[j - 1] = list->value[j];

	    list->n_values--;
	    return 0;
	}
    }

    return 0;
}

/**
 * \brief Delete list from existing list 
 *
 * \param[in,out] alist pointer to original ilist structure,
 * \param blist pointer to ilist structure with items to delete
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_list_delete_list(struct ilist *alist, const struct ilist *blist)
{
    int i;

    if (alist == NULL || blist == NULL)
	return 1;

    for (i = 0; i < blist->n_values; i++)
	Vect_list_delete(alist, blist->value[i]);

    return 0;
}

/**
 * \brief Find a given item in the list
 *
 * \param list pointer to ilist structure
 * \param val value of item
 *
 * \return 1 if an item is found
 * \return 0 no found item in the list
*/
int Vect_val_in_list(const struct ilist *list, int val)
{
    int i;

    if (list == NULL)
	return 0;

    for (i = 0; i < list->n_values; i++) {
	if (val == list->value[i])
	    return 1;
    }

    return 0;
}

/* box list routines */

/**
 * \brief Creates and initializes a struct boxlist.
 *
 * This structure is used as container for bounding boxes with id. The
 * library routines handle all memory allocation.
 *
 * \param have_boxes if set to 0, the list will hold only ids and no boxes
 * 
 * \return pointer to struct boxlist
 * \return NULL on error
 */
struct boxlist *Vect_new_boxlist(int have_boxes)
{
    struct boxlist *p;

    p = (struct boxlist *)G_malloc(sizeof(struct boxlist));

    if (p) {
	p->id = NULL;
	p->box = NULL;
	p->have_boxes = have_boxes != 0;
	p->n_values = 0;
	p->alloc_values = 0;
    }

    return p;
}

/**
 * \brief Reset boxlist structure.
 *
 * To make sure boxlist structure is clean to be re-used. List must have
 * previously been created with Vect_new_boxlist().
 *
 * \param[in,out] list pointer to struct boxlist
 * 
 * \return 0
 */
int Vect_reset_boxlist(struct boxlist *list)
{
    list->n_values = 0;

    return 0;
}

/**
 * \brief Frees all memory associated with a struct boxlist, including
 * the struct itself
 *
 * \param[in,out] list pointer to ilist structure
 */
void Vect_destroy_boxlist(struct boxlist *list)
{
    if (list) {			/* probably a moot test */
	if (list->alloc_values) {
	    G_free((void *)list->id);
	    if (list->box)
		G_free((void *)list->box);
	}
	G_free((void *)list);
    }
    list = NULL;
}

/**
 * \brief Append new item to the end of list if not yet present 
 *
 * \param[in,out] list pointer to ilist structure
 * \param id new item to append to the end of list
 * \param box bounding box
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_boxlist_append(struct boxlist *list, int id, const struct bound_box *box)
{
    int i;
    size_t size;

    if (list == NULL)
	return 1;

    for (i = 0; i < list->n_values; i++) {
	if (id == list->id[i])
	    return 0;
    }

    if (list->n_values == list->alloc_values) {
	size = (list->n_values + 1000) * sizeof(int);
	list->id = (int *)G_realloc((void *)list->id, size);

	if (list->have_boxes) {
	    size = (list->n_values + 1000) * sizeof(struct bound_box);
	    list->box = (struct bound_box *)G_realloc((void *)list->box, size);
	}

	list->alloc_values = list->n_values + 1000;
    }

    list->id[list->n_values] = id;
    if (list->have_boxes)
	list->box[list->n_values] = *box;
    list->n_values++;

    return 0;
}

/**
 * \brief Append new items to the end of list if not yet present 
 *
 * \param[in,out] alist pointer to boxlist structure where items will be appended
 * \param blist pointer to boxlist structure with new items
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_boxlist_append_boxlist(struct boxlist *alist, const struct boxlist *blist)
{
    int i;

    if (alist == NULL || blist == NULL)
	return 1;

    if (blist->have_boxes) {
	for (i = 0; i < blist->n_values; i++)
	    Vect_boxlist_append(alist, blist->id[i], &blist->box[i]);
    }
    else {
	struct bound_box box;

	box.E = box.W = box.N = box.S = box.T = box.B = 0;
	for (i = 0; i < blist->n_values; i++)
	    Vect_boxlist_append(alist, blist->id[i], &box);
    }

    return 0;
}

/**
 * \brief Remove a given value (item) from list
 *
 * \param[in,out] list pointer to boxlist structure
 * \param id to remove
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_boxlist_delete(struct boxlist *list, int id)
{
    int i, j;

    if (list == NULL)
	return 1;

    for (i = 0; i < list->n_values; i++) {
	if (id == list->id[i]) {
	    for (j = i + 1; j < list->n_values; j++) {
		list->id[j - 1] = list->id[j];
		if (list->have_boxes)
		    list->box[j - 1] = list->box[j];
	    }

	    list->n_values--;
	    return 0;
	}
    }

    return 0;
}

/**
 * \brief Delete list from existing list 
 *
 * \param[in,out] alist pointer to original boxlist structure,
 * \param blist pointer to boxlist structure with items to delete
 *
 * \return 0 on success
 * \return 1 on error
 */
int Vect_boxlist_delete_boxlist(struct boxlist *alist, const struct boxlist *blist)
{
    int i;

    if (alist == NULL || blist == NULL)
	return 1;

    for (i = 0; i < blist->n_values; i++)
	Vect_boxlist_delete(alist, blist->id[i]);

    return 0;
}

/**
 * \brief Find a given item in the list
 *
 * \param list pointer to boxlist structure
 * \param id value of item
 *
 * \return 1 if an item is found
 * \return 0 no found item in the list
*/
int Vect_val_in_boxlist(const struct boxlist *list, int id)
{
    int i;

    if (list == NULL)
	return 0;

    for (i = 0; i < list->n_values; i++) {
	if (id == list->id[i])
	    return 1;
    }

    return 0;
}
