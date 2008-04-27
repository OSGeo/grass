/**
 * \file list.c
 *
 * \brief Vector library - list definition
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Original author CERL, probably Dave Gerdes or Mike Higgins.
 * Update to GRASS 5.7 Radim Blazek and David D. Gray
 *
 * \date 2001-2008
 */

#include <stdlib.h>
#include <grass/Vect.h>
#include <grass/gis.h>

/**
 * \brief Creates and initializes a struct ilist.
 *
 * This structure is used as container for integer values. The
 * library routines handle all memory allocation.
 *
 * \return pointer to struct ilist
 * \return NULL on error
 */
struct ilist *
Vect_new_list (void)
{
  struct ilist *p;

  p = (struct ilist *) G_malloc (sizeof (struct ilist));

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
int 
Vect_reset_list (struct ilist *list)
{
  list->n_values = 0;

  return 0;
}

/**
 * \brief Frees all memory associated with a struct ilist, including
 * the struct itself
 *
 * \param[in,out] list pointer to ilist structure
 *
 * \return 0
 */
int 
Vect_destroy_list (struct ilist *list)
{
  if (list)			/* probably a moot test */
    {
      if (list->alloc_values)
	{
	  G_free ((void *) list->value);
	}
      G_free ((void *) list);
    }
  list = NULL;

  return 0;
}

/**
 * \brief Append new item to the end of list if not yet present 
 *
 * \param[in,out] list pointer to ilist structure
 * \param[in] val new item to append to the end of list
 *
 * \return 0 on success
 * \return 1 on error
 */
int
Vect_list_append ( struct ilist *list, int val )
{
    int i;
    size_t size;
    
    if ( list == NULL ) 
        return 1;
	
    for ( i = 0; i < list->n_values; i++ ) {
	if ( val == list->value[i] )
	    return 0;
    }
    
    if ( list->n_values == list->alloc_values ) {
		size = (list->n_values + 1000) * sizeof(int);
        list->value = (int *) G_realloc ( (void *) list->value, size );
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
 * \param[in] blist pointer to ilist structure with new items
 *
 * \return 0 on success
 * \return 1 on error
 */
int
Vect_list_append_list ( struct ilist *alist,  struct ilist *blist )
{
    int i;
    
    if ( alist == NULL || blist == NULL ) 
        return 1;
	
    for ( i = 0; i < blist->n_values; i++ ) 
        Vect_list_append ( alist, blist->value[i] );
    
    return 0;
}

/**
 * \brief Remove a given value (item) from list
 *
 * \param[in,out] list pointer to ilist structure
 * \param[in] val to remove
 *
 * \return 0 on success
 * \return 1 on error
 */
int
Vect_list_delete ( struct ilist *list, int val )
{
    int i, j;
    
    if ( list == NULL ) 
        return 1;
	
    for ( i = 0; i < list->n_values; i++ ) {
	if ( val == list->value[i] ) {
            for ( j = i + 1; j < list->n_values; j++ ) 
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
 * \param[in] blist pointer to ilist structure with items to delete
 *
 * \return 0 on success
 * \return 1 on error
 */
int
Vect_list_delete_list ( struct ilist *alist,  struct ilist *blist )
{
    int i;
    
    if ( alist == NULL || blist == NULL ) 
        return 1;
	
    for ( i = 0; i < blist->n_values; i++ ) 
        Vect_list_delete ( alist, blist->value[i] );
    
    return 0;
}

/**
 * \brief Find a given item in the list
 *
 * \param[in] list pointer to ilist structure
 * \param[in] val value of item
 *
 * \return 1 if an item is found
 * \return 0 no found item in the list
*/
int
Vect_val_in_list ( struct ilist *list, int val )
{
    int i;
    
    if ( list == NULL ) 
        return 0;
	
    for ( i = 0; i < list->n_values; i++ ) {
	if ( val == list->value[i] )
	    return 1;
    }
    
    return 0;
}
