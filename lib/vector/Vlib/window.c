/*!
   \file lib/vector/Vlib/window.c

   \brief Vector library - window/region

   Higher level functions for reading/writing/manipulating vectors.

   SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

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
