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

/* Init int_list */
int dig_init_list(struct ilist *list)
{
    list->value = NULL;
    list->n_values = 0;
    list->alloc_values = 0;

    return 1;
}

/* Add item to list, does not check for duplicates */
int dig_list_add(struct ilist *list, int val)
{
    if (list->n_values == list->alloc_values) {
	size_t size = (list->n_values + 1000) * sizeof(int);
	void *p = G_realloc((void *)list->value, size);

	if (p == NULL)
	    return 0;
	list->value = (int *)p;
	list->alloc_values = list->n_values + 1000;
    }

    list->value[list->n_values] = val;
    list->n_values++;

    return 1;
}
