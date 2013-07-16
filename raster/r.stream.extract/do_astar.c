#include <stdlib.h>
#include <math.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define GET_PARENT(c) ((((c) - 2) >> 3) + 1)
#define GET_CHILD(p) (((p) << 3) - 6)

HEAP_PNT heap_drop(void);
static double get_slope(CELL, CELL, double);

int do_astar(void)
{
    int r, c, r_nbr, c_nbr, ct_dir;
    GW_LARGE_INT first_cum, count;
    int nextdr[8] = { 1, -1, 0, 0, -1, 1, 1, -1 };
    int nextdc[8] = { 0, 0, -1, 1, 1, -1, 1, -1 };
    CELL ele_val, ele_up, ele_nbr[8];
    WAT_ALT wa;
    ASP_FLAG af;
    char is_in_list, is_worked;
    HEAP_PNT heap_p;
    /* sides
     * |7|1|4|
     * |2| |3|
     * |5|0|6|
     */
    int nbr_ew[8] = { 0, 1, 2, 3, 1, 0, 0, 1 };
    int nbr_ns[8] = { 0, 1, 2, 3, 3, 2, 3, 2 };
    double dx, dy, dist_to_nbr[8], ew_res, ns_res;
    double slope[8];
    struct Cell_head window;
    int skip_diag;

    count = 0;

    first_cum = n_points;

    G_message(_("A* Search..."));

    Rast_get_window(&window);

    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (r_nbr, c_nbr) for neighbours */
	r_nbr = nextdr[ct_dir];
	c_nbr = nextdc[ct_dir];
	/* account for rare cases when ns_res != ew_res */
	dy = abs(r_nbr) * window.ns_res;
	dx = abs(c_nbr) * window.ew_res;
	if (ct_dir < 4)
	    dist_to_nbr[ct_dir] = dx + dy;
	else
	    dist_to_nbr[ct_dir] = sqrt(dx * dx + dy * dy);
    }
    ew_res = window.ew_res;
    ns_res = window.ns_res;
    
    while (heap_size > 0) {
	G_percent(count++, n_points, 1);
	if (count > n_points)
	    G_fatal_error(_("BUG in A* Search: %lld surplus points"),
	                  heap_size);

	if (heap_size > n_points)
	    G_fatal_error
		(_("BUG in A* Search: too many points in heap %lld, should be %lld"),
		 heap_size, n_points);

	heap_p = heap_drop();

	r = heap_p.pnt.r;
	c = heap_p.pnt.c;

	ele_val = heap_p.ele;

	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    /* get r, c (r_nbr, c_nbr) for neighbours */
	    r_nbr = r + nextdr[ct_dir];
	    c_nbr = c + nextdc[ct_dir];
	    slope[ct_dir] = ele_nbr[ct_dir] = 0;
	    skip_diag = 0;

	    /* check that neighbour is within region */
	    if (r_nbr < 0 || r_nbr >= nrows || c_nbr < 0 || c_nbr >= ncols)
		continue;

	    seg_get(&aspflag, (char *)&af, r_nbr, c_nbr);
	    is_in_list = FLAG_GET(af.flag, INLISTFLAG);
	    is_worked = FLAG_GET(af.flag, WORKEDFLAG);
	    if (!is_worked) {
		seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
		ele_nbr[ct_dir] = wa.ele;
		slope[ct_dir] = get_slope(ele_val, ele_nbr[ct_dir],
			                  dist_to_nbr[ct_dir]);
	    }
	    /* avoid diagonal flow direction bias */
	    if (!is_in_list) {
		if (ct_dir > 3 && slope[ct_dir] > 0) {
		    if (slope[nbr_ew[ct_dir]] > 0) {
			/* slope to ew nbr > slope to center */
			if (slope[ct_dir] <
			    get_slope(ele_nbr[nbr_ew[ct_dir]],
				       ele_nbr[ct_dir], ew_res))
			    skip_diag = 1;
		    }
		    if (!skip_diag && slope[nbr_ns[ct_dir]] > 0) {
			/* slope to ns nbr > slope to center */
			if (slope[ct_dir] <
			    get_slope(ele_nbr[nbr_ns[ct_dir]],
				       ele_nbr[ct_dir], ns_res))
			    skip_diag = 1;
		    }
		}
	    }

	    if (!skip_diag) {
		if (is_in_list == 0) {
		    ele_up = ele_nbr[ct_dir];
		    af.asp = drain[r_nbr - r + 1][c_nbr - c + 1];
		    heap_add(r_nbr, c_nbr, ele_up);
		    FLAG_SET(af.flag, INLISTFLAG);
		    seg_put(&aspflag, (char *)&af, r_nbr, c_nbr);
		}
		else if (is_in_list && is_worked == 0) {
		    if (FLAG_GET(af.flag, EDGEFLAG)) {
			/* neighbour is edge in list, not yet worked */
			if (af.asp < 0) {
			    /* adjust flow direction for edge cell */
			    af.asp = drain[r_nbr - r + 1][c_nbr - c + 1];
			    seg_put(&aspflag, (char *)&af, r_nbr, c_nbr);
			}
		    }
		    else if (FLAG_GET(af.flag, DEPRFLAG)) {
			G_debug(3, "real depression");
			/* neighbour is inside real depression, not yet worked */
			if (af.asp == 0 && ele_val <= ele_nbr[ct_dir]) {
			    af.asp = drain[r_nbr - r + 1][c_nbr - c + 1];
			    FLAG_UNSET(af.flag, DEPRFLAG);
			    seg_put(&aspflag, (char *)&af, r_nbr, c_nbr);
			}
		    }
		}
	    }
	}    /* end neighbours */
	/* add astar points to sorted list for flow accumulation and stream extraction */
	first_cum--;
	seg_put(&astar_pts, (char *)&heap_p.pnt, 0, first_cum);
	seg_get(&aspflag, (char *)&af, r, c);
	FLAG_SET(af.flag, WORKEDFLAG);
	seg_put(&aspflag, (char *)&af, r, c);
    }    /* end A* search */

    G_percent(n_points, n_points, 1);	/* finish it */

    return 1;
}

/*
 * compare function for heap
 * returns 1 if point1 < point2 else 0
 */
static int heap_cmp(HEAP_PNT *a, HEAP_PNT *b)
{
    if (a->ele < b->ele)
	return 1;
    else if (a->ele == b->ele) {
	if (a->added < b->added)
	    return 1;
    }
    return 0;
}

static int sift_up(GW_LARGE_INT start, HEAP_PNT child_p)
{
    GW_LARGE_INT parent, child;
    HEAP_PNT heap_p;

    child = start;

    while (child > 1) {
	parent = GET_PARENT(child);
	seg_get(&search_heap, (char *)&heap_p, 0, parent);

	/* push parent point down if child is smaller */
	if (heap_cmp(&child_p, &heap_p)) {
	    seg_put(&search_heap, (char *)&heap_p, 0, child);
	    child = parent;
	}
	else
	    /* no more sifting up, found slot for child */
	    break;
    }

    /* add child to heap */
    seg_put(&search_heap, (char *)&child_p, 0, child);

    return 0;
}

/*
 * add item to heap
 * returns heap_size
 */
GW_LARGE_INT heap_add(int r, int c, CELL ele)
{
    HEAP_PNT heap_p;
    
    /* add point to next free position */

    heap_size++;

    heap_p.added = nxt_avail_pt;
    heap_p.ele = ele;
    heap_p.pnt.r = r;
    heap_p.pnt.c = c;

    nxt_avail_pt++;

    /* sift up: move new point towards top of heap */
    sift_up(heap_size, heap_p);

    return heap_size;
}

/*
 * drop item from heap
 * returns heap size
 */
HEAP_PNT heap_drop(void)
{
    GW_LARGE_INT child, childr, parent;
    GW_LARGE_INT i;
    HEAP_PNT child_p, childr_p, last_p, root_p;

    seg_get(&search_heap, (char *)&last_p, 0, heap_size);
    seg_get(&search_heap, (char *)&root_p, 0, 1);

    if (heap_size == 1) {
	heap_size = 0;
	return root_p;
    }

    parent = 1;
    while ((child = GET_CHILD(parent)) < heap_size) {

	seg_get(&search_heap, (char *)&child_p, 0, child);

	if (child < heap_size) {
	    childr = child + 1;
	    i = child + 8;
	    while (childr < heap_size && childr < i) {
		seg_get(&search_heap, (char *)&childr_p, 0, childr);
		if (heap_cmp(&childr_p, &child_p)) {
		    child = childr;
		    child_p = childr_p;
		}
		childr++;
	    }
	}

	if (heap_cmp(&last_p, &child_p)) {
	    break;
	}

	/* move hole down */
	seg_put(&search_heap, (char *)&child_p, 0, parent);
	parent = child;
    }

    /* fill hole */
    if (parent < heap_size) {
	seg_put(&search_heap, (char *)&last_p, 0, parent);
    }

    /* the actual drop */
    heap_size--;

    return root_p;
}

static double get_slope(CELL ele, CELL up_ele, double dist)
{
    if (ele >= up_ele)
	return 0.0;
    else
	return (double)(up_ele - ele) / dist;
}
