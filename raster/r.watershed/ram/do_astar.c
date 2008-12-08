#include "Gwater.h"
#include "do_astar.h"
#include <grass/gis.h>
#include <grass/glocale.h>

int do_astar(void)
{
    POINT *point;
    int doer, count;
    SHORT upr, upc, r, c, ct_dir;
    CELL alt_val, alt_up, asp_up, wat_val;
    CELL in_val, drain_val;
    int index_doer, index_up;


    G_message(_("SECTION 2: A * Search."));

    count = 0;
    first_astar = heap_index[1];

    /* A * Search: search uphill, get downhill path */
    while (first_astar != -1) {
	G_percent(count++, do_points, 1);

	/* start with point with lowest elevation, in case of equal elevation
	 * of following points, oldest point = point added earliest */
	/* old routine: astar_pts[first_astar] (doer = first_astar) */
	/* new routine: astar_pts[heap_index[1]] */

	doer = heap_index[1];

	point = &(astar_pts[doer]);

	/* drop astar_pts[doer] from heap */
	/* necessary to protect the current point (doer) from modification */
	/* equivalent to first_astar = point->next in old code */
	drop_pt();

	/* can go, dragged on from old code */
	first_astar = heap_index[1];

	/* downhill path for flow accumulation is set here */
	/* this path determines the order for flow accumulation calculation */
	point->nxt = first_cum;
	first_cum = doer;

	r = point->r;
	c = point->c;

	G_debug(3, "R:%2d C:%2d, ", r, c);

	index_doer = SEG_INDEX(alt_seg, r, c);
	alt_val = alt[index_doer];
	wat_val = wat[index_doer];

	FLAG_SET(worked, r, c);

	/* check all neighbours */
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
		    add_pt(upr, upc, r, c, alt_up, alt_val);
		    drain_val = drain[upr - r + 1][upc - c + 1];
		    asp[index_up] = drain_val;

		}
		else {
		    /* check if neighbour has not been worked on,
		     * update values for asp and wat */
		    in_val = FLAG_GET(worked, upr, upc);
		    if (in_val == 0) {
			asp_up = asp[index_up];
			if (asp_up < -1) {
			    asp[index_up] = drain[upr - r + 1][upc - c + 1];

			    if (wat_val > 0)
				wat[index_doer] = -wat_val;

			    replace(upr, upc, r, c);	/* alt_up used to be */
			}
		    }
		}
	    }
	}
    }
    G_percent(count, do_points, 3);	/* finish it */
    if (mfd == 0)
	flag_destroy(worked);

    flag_destroy(in_list);
    G_free(heap_index);

    return 0;
}

/* new add point routine for min heap */
int add_pt(SHORT r, SHORT c, SHORT downr, SHORT downc, CELL ele, CELL downe)
{

    FLAG_SET(in_list, r, c);

    /* add point to next free position */

    heap_size++;

    if (heap_size > do_points)
	G_fatal_error(_("heapsize too large"));

    heap_index[heap_size] = nxt_avail_pt;

    astar_pts[nxt_avail_pt].r = r;
    astar_pts[nxt_avail_pt].c = c;
    astar_pts[nxt_avail_pt].downr = downr;
    astar_pts[nxt_avail_pt].downc = downc;

    nxt_avail_pt++;

    /* sift up: move new point towards top of heap */

    sift_up(heap_size, ele);

    return 0;
}

/* new drop point routine for min heap */
int drop_pt(void)
{
    int child, childr, parent;
    CELL ele, eler;
    int i;

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
	 * older child is older startpoint for flow path, important
	 * chances are good that the left child will be the older child,
	 * but just to make sure we test */
	ele =
	    alt[SEG_INDEX
		(alt_seg, astar_pts[heap_index[child]].r,
		 astar_pts[heap_index[child]].c)];
	if (child < heap_size) {
	    childr = child + 1;
	    i = 1;
	    while (childr <= heap_size && i < 3) {
		eler =
		    alt[SEG_INDEX
			(alt_seg, astar_pts[heap_index[childr]].r,
			 astar_pts[heap_index[childr]].c)];
		if (eler < ele) {
		    child = childr;
		    ele = eler;
		    /* make sure we get the oldest child */
		}
		else if (ele == eler &&
			 heap_index[child] > heap_index[childr]) {
		    child = childr;
		}
		childr++;
		i++;
	    }
	}

	/* move hole down */

	heap_index[parent] = heap_index[child];
	parent = child;

    }

    /* hole is in lowest layer, move to heap end */
    if (parent < heap_size) {
	heap_index[parent] = heap_index[heap_size];

	ele =
	    alt[SEG_INDEX
		(alt_seg, astar_pts[heap_index[parent]].r,
		 astar_pts[heap_index[parent]].c)];
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

    child = start;
    childp = heap_index[child];

    while (child > 1) {
	parent = GET_PARENT(child);
	parentp = heap_index[parent];

	elep =
	    alt[SEG_INDEX
		(alt_seg, astar_pts[parentp].r, astar_pts[parentp].c)];
	/* parent ele higher */
	if (elep > ele) {

	    /* push parent point down */
	    heap_index[child] = parentp;
	    child = parent;

	}
	/* same ele, but parent is younger */
	else if (elep == ele && parentp > childp) {

	    /* push parent point down */
	    heap_index[child] = parentp;
	    child = parent;

	}
	else
	    /* no more sifting up, found new slot for child */
	    break;
    }

    /* set heap_index for child */
    if (child < start) {
	heap_index[child] = childp;
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

    /* find the current neighbour point and 
     * set flow direction to focus point */

    heap_run = 0;

    while (heap_run <= heap_size) {
	now = heap_index[heap_run];
	if (astar_pts[now].r == upr && astar_pts[now].c == upc) {
	    astar_pts[now].downr = r;
	    astar_pts[now].downc = c;
	    return 0;
	}
	heap_run++;
    }
    return 0;
}
