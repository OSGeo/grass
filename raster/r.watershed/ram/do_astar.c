#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int do_astar (void)
{
    POINT	*point;
    int		doer, count;
    SHORT	upr, upc, r, c, ct_dir;
    CELL	alt_val, alt_up, asp_up, wat_val;
    CELL	in_val, drain_val;

    G_message(_("SECTION 2: A * Search."));

count = 0;
while (first_astar != -1) {
    G_percent (count++, do_points, 1);
    doer = first_astar;
    point = &(astar_pts[doer]);
    first_astar = point->nxt;
    point->nxt = first_cum;

    r = astar_pts[doer].r = point->r;
    c = astar_pts[doer].c = point->c;
    G_debug(3, "R:%2d C:%2d, ", r, c);

    astar_pts[doer].downr = point->downr;
    astar_pts[doer].downc = point->downc;
    astar_pts[doer].nxt = point->nxt;
    first_cum = doer;
    FLAG_SET(worked, r, c);
    alt_val = alt[SEG_INDEX(alt_seg, r, c)];
    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
        upr = r + nextdr[ct_dir];
        upc = c + nextdc[ct_dir];
        if (upr>=0 && upr<nrows && upc>=0 && upc<ncols) {
	    in_val = FLAG_GET(in_list, upr, upc);
	    if (in_val == 0) {
		alt_up = alt[SEG_INDEX(alt_seg, upr, upc)];
                add_pt (upr, upc, r, c, alt_up, alt_val);
		drain_val = drain[upr - r + 1][upc - c + 1];
		asp[SEG_INDEX(asp_seg, upr, upc)] = drain_val;
            } else {
	        in_val = FLAG_GET(worked, upr, upc);
	        if (in_val == 0) {
		    asp_up = asp[SEG_INDEX(asp_seg, upr, upc)];
		    if (asp_up < -1) {
			asp[SEG_INDEX(asp_seg, upr, upc)] = 
				drain[upr - r + 1][upc - c + 1];
			wat_val = wat[SEG_INDEX(wat_seg, r, c)];
		        if (wat_val > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_val;
			alt_up = alt[SEG_INDEX(alt_seg, upr, upc)];
		        replace (upr, upc, r, c); /* alt_up used to be */
                    }
		}
            }
        }
    }
}
G_percent (count, do_points, 3); /* finish it */
flag_destroy (worked);
flag_destroy (in_list);

return 0;
}

int add_pt (SHORT r, SHORT c, SHORT downr, SHORT downc, CELL ele, CELL downe)
{
    int		p;
    CELL	check_ele;
    POINT	point;

    FLAG_SET(in_list, r, c);
    if (first_astar == -1) {
        astar_pts[nxt_avail_pt].r = r;
        astar_pts[nxt_avail_pt].c = c;
        astar_pts[nxt_avail_pt].downr = downr;
        astar_pts[nxt_avail_pt].downc = downc;
	astar_pts[nxt_avail_pt].nxt = -1;
	first_astar = nxt_avail_pt;
	nxt_avail_pt++;
	return 0;
    }
    p = first_astar;;
    while (1) {
	point.r = astar_pts[p].r;
	point.c = astar_pts[p].c;
	check_ele = alt[SEG_INDEX(alt_seg, point.r, point.c)];
        if (check_ele > ele) {
	    point.downr = astar_pts[p].downr;
	    point.downc = astar_pts[p].downc;
	    point.nxt = astar_pts[p].nxt;
	    astar_pts[p].r = r;
	    astar_pts[p].c = c;
	    astar_pts[p].downr = downr;
	    astar_pts[p].downc = downc;
	    astar_pts[p].nxt = nxt_avail_pt;
            astar_pts[nxt_avail_pt].r = point.r;
            astar_pts[nxt_avail_pt].c = point.c;
            astar_pts[nxt_avail_pt].downr = point.downr;
            astar_pts[nxt_avail_pt].downc = point.downc;
	    astar_pts[nxt_avail_pt].nxt = point.nxt;
	    nxt_avail_pt++;
	    return 0;
        }
	point.nxt = astar_pts[p].nxt;
        if (point.nxt == -1) {
	    astar_pts[p].nxt = nxt_avail_pt;
	    astar_pts[nxt_avail_pt].r = r;
	    astar_pts[nxt_avail_pt].c = c;
	    astar_pts[nxt_avail_pt].downr = downr;
	    astar_pts[nxt_avail_pt].downc = downc;
	    astar_pts[nxt_avail_pt].nxt = -1;
	    nxt_avail_pt++;

            return 0;
        }
        p = astar_pts[p].nxt;
    }

    return 0;
}

double 
get_slope (SHORT r, SHORT c, SHORT downr, SHORT downc, CELL ele, CELL downe)
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

int 
replace ( /* ele was in there */
    SHORT upr,
    SHORT upc,
    SHORT r,
    SHORT c
)
/* CELL ele;  */
{
    	int	now;

    	now = first_astar;
    	while (now != -1) {
		if (astar_pts[now].r == upr && astar_pts[now].c == upc) {
		    astar_pts[now].downr = r;
		    astar_pts[now].downc = c;
		    return 0;
		}
		now = astar_pts[now].nxt;
	}
	return 0;
}
