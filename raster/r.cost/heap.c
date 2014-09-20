
/****************************************************************************
 *
 * MODULE:       r.cost
 *
 * AUTHOR(S):    Antony Awaida - IESL - M.I.T.
 *               James Westervelt - CERL
 *               Pierre de Mouveaux <pmx audiovu com>
 *               Eric G. Miller <egm2 jps net>
 *
 *               min heap by Markus Metz
 *
 * PURPOSE:      Outputs a raster map layer showing the cumulative cost
 *               of moving between different geographic locations on an
 *               input raster map layer whose cell category values
 *               represent cost.
 *
 * COPYRIGHT:    (C) 2006-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

/* These routines manage the list of grid-cell candidates for 
 * visiting to calculate distances to surrounding cells.
 * A min-heap approach is used.  Components are
 * sorted first by distance then by the order in which they were added.
 *
 * insert ()
 *   inserts a new row-col with its distance value into the heap
 *
 * delete()
 *   deletes a row-col entry in the heap
 *
 * get_lowest()
 *   retrieves the entry with the smallest distance value
 */


#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "cost.h"

#define GET_PARENT(c) (((c) - 2) / 3 + 1)
#define GET_CHILD(p) (((p) * 3) - 1)

static long next_point = 0;
static long heap_size = 0;
static long heap_alloced = 0;
static struct cost **heap_index, *free_point;

int init_heap(void)
{
    next_point = 0;
    heap_size = 0;
    heap_alloced = 1000;
    heap_index = (struct cost **) G_malloc(heap_alloced * sizeof(struct cost *));

    free_point = NULL;
    
    return 0;
}

int free_heap(void)
{
    if (heap_alloced)
	G_free(heap_index);

    if (free_point)
	G_free(free_point);

    return 0;
}

/* compare two costs
 * return 1 if a < b else 0 */
int cmp_costs(struct cost *a, struct cost *b)
{
    if (a->min_cost < b->min_cost)
	return 1;
    else if (a->min_cost == b->min_cost) {
	if (a->age < b->age)
	    return 1;
    }

    return 0;
}

long sift_up(long start, struct cost * child_pnt)
{
    register long parent, child;

    child = start;

    while (child > 1) {
	parent = GET_PARENT(child);

	/* child is smaller */
	if (cmp_costs(child_pnt, heap_index[parent])) {
	    /* push parent point down */
	    heap_index[child] = heap_index[parent];
	    child = parent;
	}
	else
	    /* no more sifting up, found new slot for child */
	    break;
    }

    /* put point in new slot */
    if (child < start) {
	heap_index[child] = child_pnt;
    }

    return child;
}

struct cost *insert(double min_cost, int row, int col)
{
    struct cost *new_cell;

    if (free_point) {
	new_cell = free_point;
	free_point = NULL;
    }
    else
	new_cell = (struct cost *)(G_malloc(sizeof(struct cost)));

    new_cell->min_cost = min_cost;
    new_cell->age = next_point;
    new_cell->row = row;
    new_cell->col = col;

    next_point++;
    heap_size++;
    if (heap_size >= heap_alloced) {
	heap_alloced += 1000;
	heap_index = (struct cost **) G_realloc((void *)heap_index, heap_alloced * sizeof(struct cost *));
    }

    heap_index[heap_size] = new_cell;
    sift_up(heap_size, new_cell);

    return (new_cell);
}

struct cost *get_lowest(void)
{
    struct cost *next_cell;
    register long parent, child, childr, i;

    if (heap_size == 0)
	return NULL;
	
    next_cell = heap_index[1];
    heap_index[0] = next_cell;

    if (heap_size == 1) {
	heap_size--;

	heap_index[1] = NULL;

	return next_cell;
    }

    /* start with root */
    parent = 1;

    /* sift down: move hole back towards bottom of heap */

    while ((child = GET_CHILD(parent)) <= heap_size) {
	/* select smallest child */
	if (child < heap_size) {
	    childr = child + 1;
	    i = child + 3;
	    while (childr < i && childr <= heap_size) {
		/* get smallest child */
		if (cmp_costs(heap_index[childr], heap_index[child])) {
		    child = childr;
		}
		childr++;
	    }
	}

	/* move hole down */
	heap_index[parent] = heap_index[child];
	parent = child;
    }

    /* hole is in lowest layer, move to heap end */
    if (parent < heap_size) {
	heap_index[parent] = heap_index[heap_size];

	/* sift up last swapped point, only necessary if hole moved to heap end */
	sift_up(parent, heap_index[parent]);
    }

    /* the actual drop */
    heap_size--;

    return next_cell;
}

int delete(struct cost *delete_cell)
{
    if (free_point)
	G_free(delete_cell);
    else
	free_point = delete_cell;

    return 0;
}
