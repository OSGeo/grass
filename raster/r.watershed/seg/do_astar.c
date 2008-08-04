#include <stdlib.h>
#include <unistd.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int do_astar(void)
{
    POINT point;
    int doer, count;
    double get_slope();
    SHORT upr, upc, r, c, ct_dir;
    CELL work_val, alt_val, alt_up, asp_up, wat_val;
    CELL in_val, drain_val;
    double slope;

    G_message(_("SECTION 2: A * Search."));

    count = 0;
    while (first_astar != -1) {
	G_percent(count++, do_points, 1);
	doer = first_astar;
	seg_get(&astar_pts, (char *)&point, 0, doer);
	first_astar = point.nxt;
	point.nxt = first_cum;
	seg_put(&astar_pts, (char *)&point, 0, doer);
	first_cum = doer;
	r = point.r;
	c = point.c;
	bseg_put(&worked, &one, r, c);
	cseg_get(&alt, &alt_val, r, c);
	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    upr = r + nextdr[ct_dir];
	    upc = c + nextdc[ct_dir];
	    if (upr >= 0 && upr < nrows && upc >= 0 && upc < ncols) {
		bseg_get(&in_list, &in_val, upr, upc);
		if (in_val == 0) {
		    cseg_get(&alt, &alt_up, upr, upc);
		    add_pt(upr, upc, r, c, alt_up, alt_val);
		    drain_val = drain[upr - r + 1][upc - c + 1];
		    cseg_put(&asp, &drain_val, upr, upc);
		}
		else {
		    bseg_get(&worked, &work_val, upr, upc);
		    if (!work_val) {
			cseg_get(&asp, &asp_up, upr, upc);
			if (asp_up < -1) {
			    drain_val = drain[upr - r + 1][upc - c + 1];
			    cseg_put(&asp, &drain_val, upr, upc);
			    cseg_get(&wat, &wat_val, r, c);
			    if (wat_val > 0)
				wat_val = -wat_val;
			    cseg_put(&wat, &wat_val, r, c);
			    cseg_get(&alt, &alt_up, upr, upc);
			    replace(upr, upc, r, c);	/* alt_up used to be */
			    slope =
				get_slope(upr, upc, r, c, alt_up, alt_val);
			    dseg_put(&slp, &slope, upr, upc);
			}
		    }
		}
	    }
	}
    }
    bseg_close(&worked);
    bseg_close(&in_list);

    G_percent(count, do_points, 1);	/* finish it */
    return 0;
}

int add_pt(SHORT r, SHORT c, SHORT downr, SHORT downc, CELL ele, CELL downe)
{
    int p;
    CELL check_ele;
    POINT point, new_point;
    double slp_value, get_slope();

    if (nxt_avail_pt == do_points)
	G_fatal_error(_("problem w/ astar algorithm"));

    slp_value = get_slope(r, c, downr, downc, ele, downe);
    dseg_put(&slp, &slp_value, r, c);
    bseg_put(&in_list, &one, r, c);
    new_point.r = r;
    new_point.c = c;
    new_point.downr = downr;
    new_point.downc = downc;
    if (first_astar == -1) {
	new_point.nxt = -1;
	seg_put(&astar_pts, (char *)&new_point, 0, nxt_avail_pt);
	first_astar = nxt_avail_pt;
	nxt_avail_pt++;
	return 0;
    }
    p = first_astar;;
    while (1) {
	seg_get(&astar_pts, (char *)&point, 0, p);
	cseg_get(&alt, &check_ele, point.r, point.c);
	if (check_ele > ele) {
	    new_point.nxt = nxt_avail_pt;
	    seg_put(&astar_pts, (char *)&new_point, 0, p);
	    seg_put(&astar_pts, (char *)&point, 0, nxt_avail_pt);
	    nxt_avail_pt++;
	    return 0;
	}
	if (point.nxt == -1) {
	    point.nxt = nxt_avail_pt;
	    seg_put(&astar_pts, (char *)&point, 0, p);
	    new_point.nxt = -1;
	    seg_put(&astar_pts, (char *)&new_point, 0, nxt_avail_pt);
	    nxt_avail_pt++;

	    return 0;
	}
	p = point.nxt;
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
    int now;
    POINT point;

    now = first_astar;
    while (now != -1) {
	seg_get(&astar_pts, (char *)&point, 0, now);
	if (point.r == upr && point.c == upc) {
	    point.downr = r;
	    point.downc = c;
	    seg_put(&astar_pts, (char *)&point, 0, now);
	    return 0;
	}
	now = point.nxt;
    }
    return 0;
}
