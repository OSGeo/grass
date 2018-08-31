
/***********************************************************
 *
 *                 deleteHa.c (for spread)
 *  This routine is to delete a cell in a heap. 
 *  It 1) searches the cell backward and sequentially from 
 *        the heap (if not found, returns a error message), 
 *     2) overwrites that cell and calls fixH routine to 
 *        restore a heap order.
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "costHa.h"
#include "local_proto.h"

void
deleteHa(float old_min_cost, int row, int col,
	 struct costHa *heap, long *heap_len)
{
    /* struct costHa  *fixHa(); */
    long i;

    if (*heap_len < 1) {
	printf("programming ERROR: can't delete a cell from an ampty list");
	exit(1);
    }
    /* search the old_cell from the heap */
    for (i = 0; i <= *heap_len; i++) {
	if (heap[i].min_cost == old_min_cost &&
	    heap[i].row == row && heap[i].col == col)
	    break;
    }
    if (i == 0) {
	printf("programming ERROR: can't find the old_cell from the list");
	exit(1);
    }
    /* overwrite that cell, fix the heap */
    fixHa(i, heap, *heap_len);
    *heap_len = *heap_len - 1;

    return;
}
