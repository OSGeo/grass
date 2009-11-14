#include "Gwater.h"
#include "do_astar.h"
#include <grass/gis.h>
#include <grass/glocale.h>

double get_slope2(CELL, CELL, double);

int do_astar(void)
{
    int count;
    SHORT upr, upc, r, c, ct_dir;
    CELL alt_val, alt_nbr[8];
    CELL is_in_list, is_worked;
    int index_doer, index_up;
    /* sides
     * |7|1|4|
     * |2| |3|
     * |5|0|6|
     */
    int nbr_ew[8] = { 0, 1, 2, 3, 1, 0, 0, 1 };
    int nbr_ns[8] = { 0, 1, 2, 3, 3, 2, 3, 2 };
    double dx, dy, dist_to_nbr[8], ew_res, ns_res;
    double slope[8];

    G_message(_("SECTION 2: A * Search."));

    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (r_nbr, c_nbr) for neighbours */
	upr = nextdr[ct_dir];
	upc = nextdc[ct_dir];
	/* account for rare cases when ns_res != ew_res */
	dy = ABS(upr) * window.ns_res;
	dx = ABS(upc) * window.ew_res;
	if (ct_dir < 4)
	    dist_to_nbr[ct_dir] = dx + dy;
	else
	    dist_to_nbr[ct_dir] = sqrt(dx * dx + dy * dy);
    }
    ew_res = window.ew_res;
    ns_res = window.ns_res;

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
	    slope[ct_dir] = alt_nbr[ct_dir] = 0;
	    /* check that r, c are within region */
	    if (upr >= 0 && upr < nrows && upc >= 0 && upc < ncols) {
		index_up = SEG_INDEX(alt_seg, upr, upc);
		is_in_list = FLAG_GET(in_list, upr, upc);
		is_worked = FLAG_GET(worked, upr, upc);
		/* avoid diagonal flow direction bias */
		if (!is_worked) {
		    alt_nbr[ct_dir] = alt[index_up];
		    slope[ct_dir] =
			get_slope2(alt_val, alt_nbr[ct_dir],
				   dist_to_nbr[ct_dir]);
		    if (ct_dir > 3) {
			if (slope[nbr_ew[ct_dir]]) {
			    /* slope to ew nbr > slope to center */
			    if (slope[ct_dir] <
				get_slope2(alt_nbr[nbr_ew[ct_dir]],
					   alt_nbr[ct_dir], ew_res))
				is_in_list = 1;
			}
			if (!is_in_list && slope[nbr_ns[ct_dir]]) {
			    /* slope to ns nbr > slope to center */
			    if (slope[ct_dir] <
				get_slope2(alt_nbr[nbr_ns[ct_dir]],
					   alt_nbr[ct_dir], ns_res))
				is_in_list = 1;
			}
		    }
		}

		/* add neighbour as new point if not in the list */
		if (is_in_list == 0) {
		    add_pt(upr, upc, alt_nbr[ct_dir], alt_val);
		    /* set flow direction */
		    asp[index_up] = drain[upr - r + 1][upc - c + 1];
		}
		else {
		    if (is_worked == 0) {
			/* neighbour is edge in list, not yet worked */
			if (asp[index_up] < 0) {
			    asp[index_up] = drain[upr - r + 1][upc - c + 1];

			    if (wat[index_doer] > 0)
				wat[index_doer] = -wat[index_doer];
			}
		    }
		}
	    }    /* end if in region */
	}    /* end sides */
    }
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

double get_slope2(CELL ele, CELL up_ele, double dist)
{
    if (ele == up_ele)
	return 0.5 / dist;
    else
	return (double)(up_ele - ele) / dist;
}

/* replace is unused */
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
	    /* astar_pts[now].downr = r;
	    astar_pts[now].downc = c; */
	    return 0;
	}
	heap_run++;
    }
    return 0;
}
