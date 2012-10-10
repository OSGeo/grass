/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/vector.h>

/* Init box list */
int dig_init_boxlist(struct boxlist *list, int have_boxes)
{
    list->id = NULL;
    list->box = NULL;
    list->have_boxes = have_boxes != 0;
    list->n_values = 0;
    list->alloc_values = 0;

    return 1;
}

/* Add item to box list, does not check for duplicates */
int dig_boxlist_add(struct boxlist *list, int id, const struct bound_box *box)
{
    if (list->n_values == list->alloc_values) {
	size_t size = (list->n_values + 1000) * sizeof(int);
	void *p = G_realloc((void *)list->id, size);

	if (p == NULL)
	    return 0;
	list->id = (int *)p;

	if (list->have_boxes) {
	    size = (list->n_values + 1000) * sizeof(struct bound_box);
	    p = G_realloc((void *)list->box, size);

	    if (p == NULL)
		return 0;
	    list->box = (struct bound_box *)p;
	}

	list->alloc_values = list->n_values + 1000;
    }

    list->id[list->n_values] = id;
    if (list->have_boxes)
	list->box[list->n_values] = *box;
    list->n_values++;

    return 1;
}
