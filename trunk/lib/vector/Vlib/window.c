/*!
   \file lib/vector/Vlib/window.c

   \brief Vector library - window/region

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */
#include <grass/vector.h>

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
