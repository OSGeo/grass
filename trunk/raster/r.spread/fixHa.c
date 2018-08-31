
/**************************************************
 *
 *               fixHa.c (for spread)
 *  This routine is to restore a 'heap' order upon
 *  removing a cell from the heap, which is ordered
 *  by the min_cost of a cell.
 *
 **************************************************/
#include <stdlib.h>
#include "costHa.h"
#include "local_proto.h"

struct costHa *fixHa(long go_pos, struct costHa *heap, long heap_len)
{
    long vacant, smaller_child;

    if (heap_len == 0)
	return NULL;

    vacant = go_pos;
    while (2 * vacant <= heap_len) {
	smaller_child = 2 * vacant;
	if ((2 * vacant < heap_len) &&
	    (heap[2 * vacant + 1].min_cost < heap[2 * vacant].min_cost))
	    smaller_child++;
	if (heap[heap_len].min_cost > heap[smaller_child].min_cost) {
	    heap[vacant].min_cost = heap[smaller_child].min_cost;
	    heap[vacant].angle = heap[smaller_child].angle;
	    heap[vacant].row = heap[smaller_child].row;
	    heap[vacant].col = heap[smaller_child].col;
	    vacant = smaller_child;
	}
	else
	    break;
    }
    heap[vacant].min_cost = heap[heap_len].min_cost;
    heap[vacant].angle = heap[heap_len].angle;
    heap[vacant].row = heap[heap_len].row;
    heap[vacant].col = heap[heap_len].col;

    return heap;
}
