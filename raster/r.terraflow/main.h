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

#ifndef _main_h
#define _main_h

typedef struct {
  char* elev_grid;     /* name of input elevation grid */
  char* filled_grid;   /* name of output filled elevation grid */
  char* dir_grid;      /* name of direction grid */
  char* stats;         /* stats file */
  int   d8;            /* 1 if d8 flow model, 0 otherwise */ 
  int   mem;           /* main memory, in MB */
  int verbose;         /* 1 if verbose, 0 otherwise */
} user_options;


#endif
