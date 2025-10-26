/***********************************************************
 *
 *                 replaceHa.c (for spread)
 *  This routine is to delete a cell in a heap.
 *  It 1) searches the cell backward and sequentially from
 *        the heap (if not found, returns a error message),
 *     2) replace that cell with the new min_cost and
 *        restore a heap order.
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "costHa.h"
#include "local_proto.h"

void replaceHa(float new_min_cost, float angle, int row, int col,
               struct costHa *heap, long *heap_len)
{
    long i, smaller_child = 0;

    G_debug(4, "in replaceHa()");

    if (*heap_len < 1)
        G_fatal_error(
            "Programming ERROR: can't delete a cell from an empty list");

    /* search the cell with row and col from the heap */
    for (i = *heap_len; i >= 0; i--) {
        if (heap[i].row == row && heap[i].col == col)
            break;
    }
    if (i == 0)
        G_fatal_error(
            "Programming ERROR: can't find the old_cell from the list");

    /* replace this cell, fix the heap */
    /*take care upward */

    G_debug(4, "in replaceHa() before first while");
    while (i > 1 && new_min_cost < heap[i / 2].min_cost) {
        heap[i].min_cost = heap[i / 2].min_cost;
        heap[i].angle = heap[i / 2].angle;
        heap[i].row = heap[i / 2].row;
        heap[i].col = heap[i / 2].col;
        i = i / 2;
    }

    /*take care downward */
    if (2 * i <= *heap_len)
        smaller_child = 2 * i;

    if ((2 * i < *heap_len) &&
        (heap[2 * i].min_cost > heap[2 * i + 1].min_cost))
        smaller_child++;

    G_debug(4, "in replaceHa() before second while. smaller_child=%ld",
            smaller_child);

    while ((smaller_child <= *heap_len) && (smaller_child > 0) &&
           (new_min_cost > heap[smaller_child].min_cost)) {

        heap[i].min_cost = heap[smaller_child].min_cost;
        heap[i].angle = heap[smaller_child].angle;
        heap[i].row = heap[smaller_child].row;
        heap[i].col = heap[smaller_child].col;

        i = smaller_child;
        smaller_child = 2 * i;

        if ((2 * i < *heap_len) &&
            (heap[2 * i].min_cost > heap[2 * i + 1].min_cost))
            smaller_child++;
    }

    /*now i is the right position */
    heap[i].min_cost = new_min_cost;
    heap[i].angle = angle;
    heap[i].row = row;
    heap[i].col = col;

    G_debug(4, "replaceHa() done");

    return;
}
