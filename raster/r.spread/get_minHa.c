
/************************************************************
 *
 *                 get_minHa.c (for spread)
 *  This routine is to get the cell with the SMALLEST min_
 *  cost. This cell is the root of the (min) heap. After 
 *  removing the root, it calls fixHa routine to restore a
 *  heap order.
 *
 ************************************************************/

#include "costHa.h"
#include "local_proto.h"

void get_minHa(struct costHa *heap, struct costHa *pres_cell, long heap_len)
{
    /* struct costHa  *fixHa(); */
    if (heap_len == 0)
	return;
    pres_cell->min_cost = heap[1].min_cost;
    pres_cell->angle = heap[1].angle;
    pres_cell->row = heap[1].row;
    pres_cell->col = heap[1].col;
    fixHa(1, heap, heap_len);

    return;
}
