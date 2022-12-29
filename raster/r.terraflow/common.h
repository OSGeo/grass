/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/


#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <iostream>

#include <grass/iostream/ami.h>

#include "stats.h"
#include "option.h"
#include "types.h" /* for dimension_type */
extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}



extern statsRecorder *stats;     /* stats file */
extern userOptions *opt;          /* command-line options */
extern struct  Cell_head *region; /* header of the region */
extern dimension_type nrows, ncols;


#define MARKER(s) (stats->comment(__FILE__ ":" s))
#define STRACE(s) MARKER(s)

size_t parse_number(const char *s);

#define LM_HIST 22

#ifdef USE_LARGEMEM

class LargeMemory {
  static void *ptr[LM_HIST];
  static size_t len[LM_HIST];
  static int next;
public:
  static void *alloc(size_t);
  static void free(void *);
};

#endif /* USE_LARGEMEM */

#endif

