/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes or Mike Higgins.
 *               Update to GRASS 5.7 Radim Blazek and David D. Gray.
 *
 * PURPOSE:      Higher level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <grass/Vect.h>

/*
   int 
   Vect__get_window (
   struct Map_info *Map, double *n, double *s, double *e, double *w)
   {
   struct dig_head *dhead;

   dhead = &(Map->head);

   *n = dhead->N;
   *s = dhead->S;
   *e = dhead->E;
   *w = dhead->W;

   return (0);
   }
 */
