#include "Gwater.h"
#include "do_astar.h"
#include <grass/gis.h>
#include <grass/glocale.h>

int do_astar(void)
{
    int count;
    SHORT upr, upc, r, c, ct_dir;
    CELL alt_val, alt_up, asp_up, wat_val;
    CELL in_val, drain_val;
    int index_doer, index_up;


    G_message(_("SECTION 2: A * Search."));

    count = 0;
    first_astar = heap_index[1];
    first_cum = do_points;

    /* A* Search: search uphill, get downhill paths */
    while (heap_size > 0) {
	G_percent(count++, do_points, 1);

	/* start with point with lowest elevation, in case of equal elevation
	 * of following points, oldest point = point added earliest */
	index_doer = astar_pts[1];

	drop_pt();

	/* add astar points to sorted list for flow accumulation */
	astar_pts[first_cum] = index_doer;
	first_cum--;

	seg_index_rc(alt_seg, index_doer, &r, &c);

	G_debug(3, "A* Search: row %d, column %d, ", r, c);

	alt_val = alt[index_doer];

	FLAG_SET(worked, r, c);

	/* check neighbours */
	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    /* get r, c (upr, upc) for this neighbour */
	    upr = r + nextdr[ct_dir];
	    upc = c + nextdc[ct_dir];
	    index_up = SEG_INDEX(alt_seg, upr, upc);
	    /* check that r, c are within region */
	    if (upr >= 0 && upr < nrows && upc >= 0 && upc < ncols) {
		/* check if neighbour is in the list */
		/* if not, add as new point */
		in_val = FLAG_GET(in_list, upr, upc);
		if (in_val == 0) {
		    alt_up = alt[index_up];
		    /* flow direction is set here */
		    add_pt(upr, upc, alt_up, alt_val);
		    drain_val = drain[upr - r + 1][upc - c + 1];
		    asp[index_up] = drain_val;
		}
		else {
		    in_val = FLAG_GET(worked, upr, upc);
		    /* neighbour is edge in list, not yet worked */
		    if (in_val == 0) {
			asp_up = asp[index_up];
			if (asp_up < 0) {
			    asp[index_up] = drain[upr - r + 1][upc - c + 1];

			    wat_val = wat[index_doer];
			    if (wat_val > 0)
				wat[index_doer] = -wat_val;
			}
		    }
		}
	    }
	}
    }
    /* this was a lot of indentation, improve code aesthetics? */
    G_percent(count, do_points, 1);	/* finish it */
    if (mfd == 0)
	flag_destroy(worked);

    flag_destroy(in_list);
    G_free(heap_index);

    return 0;
}

/* compare two heap points */
/* return 1 if a < b else 0 */
int cmp_pnt(CELL elea, CELL eleb, int addeda, int addedb)
{
    if (elea < eleb)
	return 1;
    else if (elea == eleb) {
	if (addeda < addedb)
	    return 1;
    }
    return 0;
}

/* new add point routine for min heap */
int add_pt(SHORT r, SHORT c, CELL ele, CELL downe)
{

    FLAG_SET(in_list, r, c);

    /* add point to next free position */

    heap_size++;

    if (heap_size > do_points)
	G_fatal_error(_("heapsize too large"));

    heap_index[heap_size] = nxt_avail_pt++;

    astar_pts[heap_size] = SEG_INDEX(alt_seg, r, c);

    /* sift up: move new point towards top of heap */

    sift_up(heap_size, ele);

    return 0;
}

/* new drop point routine for min heap */
int drop_pt(void)
{
    register int child, childr, parent;
    CELL ele, eler;
    register int i;

    if (heap_size == 1) {
	heap_index[1] = -1;
	heap_size = 0;
	return 0;
    }

    /* start with root */
    parent = 1;

    /* sift down: move hole back towards bottom of heap */
    /* sift-down routine customised for A * Search logic */

    while ((child = GET_CHILD(parent)) <= heap_size) {
	/* select child with lower ele, if equal, older child
	 * older child is older startpoint for flow path, important */
	ele = alt[astar_pts[child]];
	if (child < heap_size) {
	    childr = child + 1;
	    i = child + 3;
	    while (childr <= heap_size && childr < i) {
		eler = alt[astar_pts[childr]];
		/* get smallest child */
		if (cmp_pnt(eler, ele, heap_index[childr], heap_index[child])) {
		    child = childr;
		    ele = eler;
		}
		childr++;
	    }
	    /* break if childr > last entry? that saves sifting up again
	     * OTOH, this is another comparison
	     * we have a max heap height of 20: log(INT_MAX)/log(n children per node)
	     * that would give us in the worst case 20*2 additional comparisons with 3 children
	     * the last entry will never go far up again, less than half the way
	     * so the additional comparisons for going all the way down
	     * and then a bit up again are likely less than 20*2 */
	    /* find the error in this reasoning */
	}

	/* move hole down */

	heap_index[parent] = heap_index[child];
	astar_pts[parent] = astar_pts[child];
	parent = child;

    }

    /* hole is in lowest layer, move to heap end */
    if (parent < heap_size) {
	heap_index[parent] = heap_index[heap_size];
	astar_pts[parent] = astar_pts[heap_size];

	ele = alt[astar_pts[parent]];
	/* sift up last swapped point, only necessary if hole moved to heap end */
	sift_up(parent, ele);
    }

    /* the actual drop */
    heap_size--;

    return 0;
}

/* standard sift-up routine for d-ary min heap */
int sift_up(int start, CELL ele)
{
    register int parent, child, child_idx, child_added;
    CELL elep;

    child = start;
    child_added = heap_index[child];
    child_idx = astar_pts[child];

    while (child > 1) {
	parent = GET_PARENT(child);

	elep = alt[astar_pts[parent]];
	/* child smaller */
	if (cmp_pnt(ele, elep, child_added, heap_index[parent])) {
	    /* push parent point down */
	    heap_index[child] = heap_index[parent];
	    astar_pts[child] = astar_pts[parent];
	    child = parent;
	}
	else
	    /* no more sifting up, found new slot for child */
	    break;
    }

    /* put point in new slot */
    if (child < start) {
	heap_index[child] = child_added;
	astar_pts[child] = child_idx;
    }

    return 0;

}

double
get_slope(SHORT r, SHORT c, SHORT downr, SHORT downc, CELL ele, CELL downe)
{
    double slope;

    if (r == downr)
	slope = (ele - downe) / window.ew_res;
    else if (c == downc)
	slope = (ele - downe) / window.ns_res;
    else
	slope = (ele - downe) / diag;

    if (slope < MIN_SLOPE)
	return (MIN_SLOPE);

    return (slope);
}


/* new replace */
int replace(			/* ele was in there */
	       SHORT upr, SHORT upc, SHORT r, SHORT c)
/* CELL ele;  */
{
    int now, heap_run;
    int r2, c2;

    /* find the current neighbour point and 
     * set flow direction to focus point */

    heap_run = 0;

    while (heap_run <= heap_size) {
	now = heap_index[heap_run];
	/* if (astar_pts[now].r == upr && astar_pts[now].c == upc) { */
	seg_index_rc(alt_seg, astar_pts[now], &r2, &c2);
	if (r2 == upr && c2 == upc) {
	    /*astar_pts[now].downr = r;
	    astar_pts[now].downc = c; */
	    return 0;
	}
	heap_run++;
    }
    return 0;
}
