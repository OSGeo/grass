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



#ifndef option__h
#define option__h


#define OUTPUT_TCI
/* #define SAVE_ASCII */


typedef struct {
  char* elev_grid;     /* name of input elevation grid */

  char* filled_grid;   /* name of output filled elevation grid */
  char* dir_grid;      /* name of output direction grid */
  char* watershed_grid;/* name of output watershed grid */
  char* flowaccu_grid; /* name of output flow accumulation grid */
#ifdef OUTPUT_TCI
  char* tci_grid; 
#endif

  int   d8;            /* 1 if d8 flow model, 0 otherwise */ 
  float d8cut;           /* flow value where flow accu comp switches to D8 */


  int   mem;           /* main memory, in MB */
  char* streamdir;     /* location of temposary STREAMs */

  char* stats;         /* stats file */
  int verbose;         /* 1 if verbose, 0 otherwise */

} userOptions;





#endif

