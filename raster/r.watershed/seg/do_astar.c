#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "Gwater.h"
#include "do_astar.h"

int do_astar(void)
{
    POINT point;
    int doer, count;
    SHORT upr, upc, r, c, ct_dir;
    CELL work_val, alt_val, alt_up, asp_up;
    DCELL wat_val;
    CELL in_val, drain_val;
    HEAP heap_pos;

    /* double slope; */

    G_message(_("SECTION 2: A * Search."));

    count = 0;
    seg_get(&heap_index, (char *)&heap_pos, 0, 1);
    first_astar = heap_pos.point;

    /* A * Search: downhill path through all not masked cells */
    while (first_astar != -1) {
	G_percent(count++, do_points, 1);

	seg_get(&heap_index, (char *)&heap_pos, 0, 1);
	doer = heap_pos.point;

	seg_get(&astar_pts, (char *)&point, 0, doer);

	/* drop astar_pts[doer] from heap */
	drop_pt();

	seg_get(&heap_index, (char *)&heap_pos, 0, 1);
	first_astar = heap_pos.point;

	point.nxt = first_cum;
	seg_put(&astar_pts, (char *)&point, 0, doer);

	first_cum = doer;
	r = point.r;
	c = point.c;

	bseg_put(&worked, &one, r, c);
	cseg_get(&alt, &alt_val, r, c);

	/* check all neighbours, breadth first search */
	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    /* get r, c (upr, upc) for this neighbour */
	    upr = r + nextdr[ct_dir];
	    upc = c + nextdc[ct_dir];
	    /* check that upr, upc are within region */
	    if (upr >= 0 && upr < nrows && upc >= 0 && upc < ncols) {
		/* check if neighbour is in the list */
		/* if not, add as new point */
		bseg_get(&in_list, &in_val, upr, upc);
		if (in_val == 0) {
		    cseg_get(&alt, &alt_up, upr, upc);
		    add_pt(upr, upc, r, c, alt_up, alt_val);
		    /* flow direction is set here */
		    drain_val = drain[upr - r + 1][upc - c + 1];
		    cseg_put(&asp, &drain_val, upr, upc);
		}
		else {
		    /* check if neighbour has not been worked on,
		     * update values for asp, wat and slp */
		    bseg_get(&worked, &work_val, upr, upc);
		    if (!work_val) {
			cseg_get(&asp, &asp_up, upr, upc);
			if (asp_up < -1) {
			    drain_val = drain[upr - r + 1][upc - c + 1];
			    cseg_put(&asp, &drain_val, upr, upc);
			    dseg_get(&wat, &wat_val, r, c);
			    if (wat_val > 0)
				wat_val = -wat_val;
			    dseg_put(&wat, &wat_val, r, c);
			    cseg_get(&alt, &alt_up, upr, upc);
			    replace(upr, upc, r, c);	/* alt_up used to be */
			    /* slope = get_slope (upr, upc, r, c, alt_up, alt_val);
			       dseg_put (&slp, &slope, upr, upc); */
			}
		    }
		}
	    }
	}
    }

    if (mfd == 0)
	bseg_close(&worked);

    bseg_close(&in_list);
    seg_close(&heap_index);

    G_percent(count, do_points, 1);	/* finish it */
    return 0;
}

/* new add point routine for min heap */
int add_pt(SHORT r, SHORT c, SHORT downr, SHORT downc, CELL ele, CELL downe)
{
    POINT point;
    HEAP heap_pos;

    /* double slp_value; */

    /* slp_value = get_slope(r, c, downr, downc, ele, downe);
       dseg_put (&slp, &slp_value, r, c); */
    bseg_put(&in_list, &one, r, c);

    /* add point to next free position */

    heap_size++;

    if (heap_size > do_points)
	G_fatal_error(_("heapsize too large"));

    heap_pos.point = nxt_avail_pt;
    heap_pos.ele = ele;
    seg_put(&heap_index, (char *)&heap_pos, 0, heap_size);

    point.r = r;
    point.c = c;
    point.downr = downr;
    point.downc = downc;

    seg_put(&astar_pts, (char *)&point, 0, nxt_avail_pt);

    nxt_avail_pt++;

    /* sift up: move new point towards top of heap */

    sift_up(heap_size, ele);

    return 0;
}

/* drop point routine for min heap */
int drop_pt(void)
{
    int child, childr, parent;
    int childp, childrp;
    CELL ele, eler;
    int i;
    HEAP heap_pos;

    if (heap_size == 1) {
	parent = -1;
	heap_pos.point = -1;
	heap_pos.ele = 0;
	seg_put(&heap_index, (char *)&heap_pos, 0, 1);
	heap_size = 0;
	return 0;
    }

    /* start with heap root */
    parent = 1;

    /* sift down: move hole back towards bottom of heap */
    /* sift-down routine customised for A * Search logic */

    while ((child = GET_CHILD(parent)) <= heap_size) {
	/* select child with lower ele, if both are equal, older child
	 * older child is older startpoint for flow path, important */
	seg_get(&heap_index, (char *)&heap_pos, 0, child);
	childp = heap_pos.point;
	ele = heap_pos.ele;
	if (child < heap_size) {
	    childr = child + 1;
	    i = 1;
	    while (childr <= heap_size && i < 3) {
		seg_get(&heap_index, (char *)&heap_pos, 0, childr);
		childrp = heap_pos.point;
		eler = heap_pos.ele;
		if (eler < ele) {
		    child = childr;
		    childp = childrp;
		    ele = eler;
		}
		/* make sure we get the oldest child */
		else if (ele == eler && childp > childrp) {
		    child = childr;
		    childp = childrp;
		}
		childr++;
		i++;
	    }
	}

	/* move hole down */
	heap_pos.point = childp;
	heap_pos.ele = ele;
	seg_put(&heap_index, (char *)&heap_pos, 0, parent);
	parent = child;

    }

    /* hole is in lowest layer, move to heap end */
    if (parent < heap_size) {
	seg_get(&heap_index, (char *)&heap_pos, 0, heap_size);
	seg_put(&heap_index, (char *)&heap_pos, 0, parent);

	ele = heap_pos.ele;

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
    int parent, parentp, child, childp;
    CELL elep;
    HEAP heap_pos;

    child = start;
    seg_get(&heap_index, (char *)&heap_pos, 0, child);
    childp = heap_pos.point;

    while (child > 1) {
	parent = GET_PARENT(child);
	seg_get(&heap_index, (char *)&heap_pos, 0, parent);
	parentp = heap_pos.point;
	elep = heap_pos.ele;

	/* parent ele higher */
	if (elep > ele) {

	    /* push parent point down */
	    seg_put(&heap_index, (char *)&heap_pos, 0, child);
	    child = parent;

	}
	/* same ele, but parent is younger */
	else if (elep == ele && parentp > childp) {

	    /* push parent point down */
	    seg_put(&heap_index, (char *)&heap_pos, 0, child);
	    child = parent;

	}
	else
	    /* no more sifting up, found new slot for child */
	    break;
    }

    /* no more sifting up, found new slot for child */
    if (child < start) {
	heap_pos.point = childp;
	heap_pos.ele = ele;
	seg_put(&heap_index, (char *)&heap_pos, 0, child);
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

int replace(			/* ele was in there */
	       SHORT upr, SHORT upc, SHORT r, SHORT c)
/* CELL ele;  */
{
    int now, heap_run;
    POINT point;
    HEAP heap_pos;

    heap_run = 0;

    /* this is dumb, works for ram, for seg make new index pt_index[row][col] */
    while (heap_run <= heap_size) {
	seg_get(&heap_index, (char *)&heap_pos, 0, heap_run);
	now = heap_pos.point;
	seg_get(&astar_pts, (char *)&point, 0, now);
	if (point.r == upr && point.c == upc) {
	    point.downr = r;
	    point.downc = c;
	    seg_put(&astar_pts, (char *)&point, 0, now);
	    return 0;
	}
	heap_run++;;
    }
    return 0;
}
