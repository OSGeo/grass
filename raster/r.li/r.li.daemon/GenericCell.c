/*
 * AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include "GenericCell.h"

int equalsGenericCell(generic_cell c1, generic_cell c2)
{
    if ((c1.t) != (c2.t)) {
	return GC_DIFFERENT_TYPE;
    }
    switch (c1.t) {
    case CELL_TYPE:
	{
	    if (((c1.val).c) > ((c2.val).c))
		return GC_HIGHER;
	    else {
		if (((c1.val).c) == ((c2.val).c))
		    return GC_EQUAL;
		else
		    return GC_LOWER;
	    }
	    break;
	}
    case DCELL_TYPE:
	{
	    if (((c1.val).dc) > ((c2.val).dc))
		return GC_HIGHER;
	    else {
		if (((c1.val).dc) == ((c2.val).dc))
		    return GC_EQUAL;
		else
		    return GC_LOWER;
	    }
	    break;
	}
    case FCELL_TYPE:
	{
	    if (((c1.val).fc) > ((c2.val).fc))
		return GC_HIGHER;
	    else {
		if (((c1.val).fc) == ((c2.val).fc))
		    return GC_EQUAL;
		else
		    return GC_LOWER;
	    }
	    break;
	}
    default:
	{
	    return GC_ERR_UNKNOWN;
	    break;
	}
    }
}


void printGenericCell(generic_cell c)
{
    switch (c.t) {
    case CELL_TYPE:
	{
	    printf("\n    genericCell_print:c.val.c=%d", (c.val).c);
	    fflush(stdout);
	    break;
	}
    case DCELL_TYPE:
	{
	    printf("\n    genericCell_print:c.val.dc=%f", (c.val).dc);
	    fflush(stdout);
	    break;
	}
    case FCELL_TYPE:
	{
	    printf("\n    genericCell_print:c.val.fc=%f", (c.val).fc);
	    fflush(stdout);
	    break;
	}
    default:
	{
	    G_fatal_error("printUnionCel: Wrong type");
	    break;
	}
    }
}
