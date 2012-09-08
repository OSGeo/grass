/*
 ****************************************************************************
 *
 * MODULE:       gis library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading and manipulating integer list
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/gis.h>

/** 
 * Init an integer list  
 *
 * \param list The pointer to an integer list
 *
 * */
void G_init_ilist(struct ilist *list)
{
    list->value = NULL;
    list->n_values = 0;
    list->alloc_values = 0;
}

/** 
 * \brief Add item to ilist
 *
 *  This function adds an integer to the list but does not check for duplicates.
 *  In case reallocation fails, G_fatal_error() will be invoked by the
 *  allocation function.
 *
 * \param list The ilist pointer
 * \param val The value to attach
 *
 * */
void G_ilist_add(struct ilist *list, int val)
{
    if (list->n_values == list->alloc_values) {
	size_t size = (list->n_values + 1000) * sizeof(int);
	void *p = G_realloc((void *)list->value, size);

	list->value = (int *)p;
	list->alloc_values = list->n_values + 1000;
    }

    list->value[list->n_values] = val;
    list->n_values++;
}
