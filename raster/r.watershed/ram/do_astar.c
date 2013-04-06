#include "Gwater.h"
#include "do_astar.h"
#include <grass/gis.h>
#include <grass/glocale.h>

static double get_slope2(CELL, CELL, double);

int do_astar(void)
{
    int count;
    int upr, upc, r, c, ct_dir;
    CELL alt_val, alt_nbr[8];
    CELL is_in_list, is_worked, flat_is_done, nbr_flat_is_done;
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
    int skip_diag;
    CELL *alt_bak;

    G_message(_("SECTION 2: A* Search."));

    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (upr, upc) for neighbours */
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

    if (flat_flag) {
	alt_bak =
	    (CELL *) G_malloc(sizeof(CELL) * size_array(&alt_seg, nrows, ncols));
	flat_done = flag_create(nrows, ncols);
	flat_is_done = 0;

	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		index_doer = SEG_INDEX(alt_seg, r, c);
		alt_bak[index_doer] = alt[index_doer];

		flag_unset(flat_done, r, c);
	    }
	}
    }
    else {
	alt_bak = NULL;
	flat_done = NULL;
	flat_is_done = 1;
    }

    /* A* Search: search uphill, get downhill paths */
    while (heap_size > 0) {
	G_percent(count++, do_points, 1);

	/* get point with lowest elevation, in case of equal elevation
	 * of following points, oldest point = point added earliest */
	index_doer = astar_pts[1];

	drop_pt();

	/* add astar points to sorted list for flow accumulation */
	astar_pts[first_cum] = index_doer;
	first_cum--;

	seg_index_rc(alt_seg, index_doer, &r, &c);

	G_debug(3, "A* Search: row %d, column %d, ", r, c);

	alt_val = alt[index_doer];

	if (flat_flag) {
	    flat_is_done = FLAG_GET(flat_done, r, c);
	}

	/* check all neighbours, breadth first search */
	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    /* get r, c (upr, upc) for this neighbour */
	    upr = r + nextdr[ct_dir];
	    upc = c + nextdc[ct_dir];
	    slope[ct_dir] = alt_nbr[ct_dir] = 0;
	    /* check if r, c are within region */
	    if (upr >= 0 && upr < nrows && upc >= 0 && upc < ncols) {
		index_up = SEG_INDEX(alt_seg, upr, upc);
		is_in_list = FLAG_GET(in_list, upr, upc);
		is_worked = FLAG_GET(worked, upr, upc);
		skip_diag = 0;
		
		alt_nbr[ct_dir] = alt[index_up];
		if (flat_flag && !is_in_list && !is_worked) {
		    alt_val = alt_bak[index_doer];
		    alt_nbr[ct_dir] = alt_bak[index_up];
		    if (!flat_is_done && alt_nbr[ct_dir] == alt_val) {
			do_flatarea(index_doer, alt_val, alt_bak, alt);
			alt_nbr[ct_dir] = alt[index_up];
			flat_is_done = 1;
			nbr_flat_is_done = 1;
		    }
		    nbr_flat_is_done = FLAG_GET(flat_done, upr, upc);
		    if (!nbr_flat_is_done) {
			/* use original ele values */
			alt_val = alt_bak[index_doer];
			alt_nbr[ct_dir] = alt_bak[index_up];
		    }
		    else {
			/* use modified ele values */
			alt_val = alt[index_doer];
			alt_nbr[ct_dir] = alt[index_up];
		    }
		}
		
		/* avoid diagonal flow direction bias */
		if (!is_worked) {
		    slope[ct_dir] =
			get_slope2(alt_val, alt_nbr[ct_dir],
				   dist_to_nbr[ct_dir]);
		}
		if (!is_in_list) {
		    if (ct_dir > 3 && slope[ct_dir] > 0) {
			if (slope[nbr_ew[ct_dir]] > 0) {
			    /* slope to ew nbr > slope to center */
			    if (slope[ct_dir] <
				get_slope2(alt_nbr[nbr_ew[ct_dir]],
					   alt_nbr[ct_dir], ew_res))
				skip_diag = 1;
			}
			if (!skip_diag && slope[nbr_ns[ct_dir]] > 0) {
			    /* slope to ns nbr > slope to center */
			    if (slope[ct_dir] <
				get_slope2(alt_nbr[nbr_ns[ct_dir]],
					   alt_nbr[ct_dir], ns_res))
				skip_diag = 1;
			}
		    }
		}

		if (!skip_diag) {
		    /* add neighbour as new point if not in the list */
		    if (is_in_list == 0) {
			add_pt(upr, upc, alt_nbr[ct_dir]);
			/* set flow direction */
			asp[index_up] = drain[upr - r + 1][upc - c + 1];
		    }
		    else if (is_in_list && is_worked == 0) {
			/* neighbour is edge in list, not yet worked */
			if (asp[index_up] < 0) {
			    asp[index_up] = drain[upr - r + 1][upc - c + 1];

			    if (wat[index_doer] > 0)
				wat[index_doer] = -1.0 * wat[index_doer];
			}
			/* neighbour is inside real depression, not yet worked */
			else if (asp[index_up] == 0) {
			    asp[index_up] = drain[upr - r + 1][upc - c + 1];
			}
		    }
		}
	    }    /* end if in region */
	}    /* end sides */
	FLAG_SET(worked, r, c);
    }
    G_percent(count, do_points, 1);	/* finish it */
    if (mfd == 0)
	flag_destroy(worked);

    flag_destroy(in_list);
    G_free(heap_index);

    if (flat_flag) {
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		index_doer = SEG_INDEX(alt_seg, r, c);
		alt[index_doer] = alt_bak[index_doer];
	    }
	}
	G_free(alt_bak);
	flag_destroy(flat_done);
    }

    return 0;
}

/* compare two heap points */
/* return 1 if a < b else 0 */
static int cmp_pnt(CELL elea, CELL eleb, int addeda, int addedb)
{
    if (elea == eleb) {
	return (addeda < addedb);
    }
    return (elea < eleb);
}

/* standard sift-up routine for d-ary min heap */
static int sift_up(int start, CELL ele)
{
    register int parent, child, child_idx, child_added;
    CELL elep;

    child = start;
    child_added = heap_index[child];
    child_idx = astar_pts[child];

    while (child > 1) {
	GET_PARENT(parent, child);

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

/* add point routine for min heap */
int add_pt(int r, int c, CELL ele)
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

/* drop point routine for min heap */
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

    while (GET_CHILD(child, parent) <= heap_size) {
	/* select child with lower ele, if both are equal, older child
	 * older child is older startpoint for flow path, important */
	ele = alt[astar_pts[child]];
	i = child + 3;
	for (childr = child + 1; childr <= heap_size && childr < i; childr++) {
	    eler = alt[astar_pts[childr]];
	    if (cmp_pnt(eler, ele, heap_index[childr], heap_index[child])) {
		child = childr;
		ele = eler;
	    }
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

double
get_slope(int r, int c, int downr, int downc, CELL ele, CELL downe)
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

static double get_slope2(CELL ele, CELL up_ele, double dist)
{
    if (ele >= up_ele)
	return 0.0;
    else
	return (double)(up_ele - ele) / dist;
}
