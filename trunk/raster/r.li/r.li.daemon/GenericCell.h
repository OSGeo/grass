/*
 * AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#ifndef GENERICCELL_H
#define GENERICCELL_H

#include <grass/gis.h>
#include <grass/raster.h>

#define GC_HIGHER 1
#define GC_EQUAL 2
#define GC_LOWER 3
#define GC_DIFFERENT_TYPE 0
#define GC_ERR_UNKNOWN -1

typedef union cella
{
    CELL c;
    DCELL dc;
    FCELL fc;
} cella;

typedef struct generic_cell
{
    int t;
    cella val;
} generic_cell;

#endif

void printGenericCell(generic_cell c);
int equalsGenericCell(generic_cell c1, generic_cell c2);
