
/***************************************************************************
 *
 * MODULE:    r.fill.stats
 * FILE:      cell_funcs.h
 * AUTHOR(S): Benjamin Ducke
 * PURPOSE:   See cell_funcs.c
 *
 * COPYRIGHT: (C) 2011 by the GRASS Development Team
 *
 *            This program is free software under the GPL (>=v2)
 *            Read the file COPYING that comes with GRASS for details.
 *
 ****************************************************************************
 */

#ifndef CELL_FUNCS_H
#define CELL_FUNCS_H

#include <grass/raster.h>

void init_cell_funcs();

extern RASTER_MAP_TYPE IN_TYPE; /* stuff for cell input and output data */
extern RASTER_MAP_TYPE OUT_TYPE;

/* below are the sizes (in bytes) for the different GRASS cell types */
extern unsigned char CELL_IN_SIZE;
extern unsigned char CELL_IN_PTR_SIZE;
extern unsigned char CELL_OUT_SIZE;
extern unsigned char CELL_OUT_PTR_SIZE;
extern unsigned char CELL_ERR_SIZE;

extern void (*WRITE_CELL_VAL) (void *, void *); /* write a cell value of any type into an output cell */
extern void (*WRITE_DOUBLE_VAL) (void *, double);       /* write a double value into an output cell */
extern int (*IS_NULL) (void *); /* check if a cell is "no data" */
extern void (*SET_NULL) (void *, unsigned long);        /* write null value(s) */

#endif /* CELL_FUNCS_H */
