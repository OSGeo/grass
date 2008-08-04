
/***********************************************************
 *
 *                 insertHa.c (for spread)
 *  This routine is to insert a new cell into a heap. It is
 *  a min_heap, the heap order is by the min_cost of cells,
 *  and the min_cost of the heap root is always the SMALLEST.
 *
 ************************************************************/

#include "costHa.h"
#include "local_proto.h"

void
insertHa(float new_min_cost, float angle, int row, int col,
	 struct costHa *heap, long *heap_len)
{
    long vacant;

    vacant = ++*heap_len;
    while (vacant > 1 && new_min_cost < heap[vacant / 2].min_cost) {
	heap[vacant].min_cost = heap[vacant / 2].min_cost;
	heap[vacant].angle = heap[vacant / 2].angle;
	heap[vacant].row = heap[vacant / 2].row;
	heap[vacant].col = heap[vacant / 2].col;
	vacant = vacant / 2;
    }
    heap[vacant].min_cost = new_min_cost;
    heap[vacant].angle = angle;
    heap[vacant].row = row;
    heap[vacant].col = col;
    return;
}
