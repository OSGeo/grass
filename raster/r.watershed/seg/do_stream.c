#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int do_stream(void)
{
    int r, c, dr, dc;
    char is_swale;
    DCELL value, *wat_nbr;
    WAT_ALT wa;
    POINT point;
    int killer, threshold, count;

    /* Breadth First Search */
    int stream_cells, swale_cells;
    int r_nbr, c_nbr, r_max, c_max, ct_dir, np_side;
    CELL ele, *ele_nbr, asp_val, asp_val_down;
    double max_acc;
    int edge;
    char *flag_nbr, this_flag_value, flag_value;
    int workedon;
    SHORT asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    SHORT asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };

    G_message(_("SECTION 4: Extracting Streams."));

    flag_nbr = (char *)G_malloc(sides * sizeof(char));
    wat_nbr = (DCELL *)G_malloc(sides * sizeof(DCELL));
    ele_nbr = (CELL *)G_malloc(sides * sizeof(CELL));

    workedon = 0;

    count = 0;
    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;

    for (killer = 0; killer < do_points; killer++) {
	G_percent(count++, do_points, 1);
	seg_get(&astar_pts, (char *)&point, 0, killer);
	r = point.r;
	c = point.c;
	cseg_get(&asp, &asp_val, r, c);
	if (asp_val) {
	    dr = r + asp_r[ABS(asp_val)];
	    dc = c + asp_c[ABS(asp_val)];
	}
	else
	    dr = dc = -1;
	if (dr >= 0 && dr < nrows && dc >= 0 && dc < ncols) {
	    bseg_get(&bitflags, &this_flag_value, r, c);
	    /* do not continue streams along edges, this causes artifacts */
	    if (FLAG_GET(this_flag_value, EDGEFLAG)) {
		is_swale = FLAG_GET(this_flag_value, SWALEFLAG);
		if (is_swale && asp_val > 0) {

		    /* find first neighbour that is NULL or outside region */
		    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
			r_nbr = r + nextdr[ct_dir];
			c_nbr = c + nextdc[ct_dir];

			/* check if neighbour is within region */
			if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
			    c_nbr < ncols) {

			    bseg_get(&bitflags, &flag_value, r_nbr, c_nbr);
			    if (FLAG_GET(flag_value, NULLFLAG)) {
				asp_val = -1 * drain[r - r_nbr + 1][c - c_nbr + 1];
				break;
			    }
			}
			else {
			    asp_val = -1 * drain[r - r_nbr + 1][c - c_nbr + 1];
			    break;
			}
		    }
		    cseg_put(&asp, &asp_val, r, c);
		}
		continue;
	    }

	    seg_get(&watalt, (char *)&wa, r, c);
	    value = wa.wat;
	    if (point.guessed && value > 0)
		value = -value;

	    r_max = dr;
	    c_max = dc;

	    np_side = -1;
	    stream_cells = 0;
	    swale_cells = 0;
	    max_acc = -1;
	    ele = wa.ele;
	    edge = 0;
	    /* visit all neighbours */
	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];

		wat_nbr[ct_dir] = 0;
		ele_nbr[ct_dir] = 0;
		FLAG_SET(flag_nbr[ct_dir], WORKEDFLAG);

		if (dr == r_nbr && dc == c_nbr)
		    np_side = ct_dir;

		/* check if neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols) {

		    /* check for swale or stream cells */
		    bseg_get(&bitflags, &flag_nbr[ct_dir], r_nbr, c_nbr);
		    is_swale = FLAG_GET(flag_nbr[ct_dir], SWALEFLAG);
		    if (is_swale)
			swale_cells++;
		    seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
		    wat_nbr[ct_dir] = wa.wat;

		    if (fabs(wat_nbr[ct_dir]) >= threshold)
			stream_cells++;

		    if (!FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG)) {

			ele_nbr[ct_dir] = wa.ele;
			
			edge = FLAG_GET(flag_nbr[ct_dir], NULLFLAG);
			if (!edge && ele_nbr[ct_dir] <= ele) {

			    /* set main drainage direction */
			    if (fabs(wat_nbr[ct_dir]) >= max_acc) {
				max_acc = fabs(wat_nbr[ct_dir]);
				r_max = r_nbr;
				c_max = c_nbr;
			    }
			}
		    }
		    else if (ct_dir == np_side && !edge) {
			/* check for consistency with main drainage direction */
			workedon++;
		    }
		}
		else
		    edge = 1;
		if (edge)
		    break;
	    }
	    /* do not continue streams along edges, this causes artifacts */
	    if (edge) {
		is_swale = FLAG_GET(this_flag_value, SWALEFLAG);
		if (is_swale && asp_val > 0) {
		    asp_val = -1 * drain[r - r_nbr + 1][c - c_nbr + 1];
		    cseg_put(&asp, &asp_val, r, c);
		}
		continue;
	    }

	    /* adjust main drainage direction to A* path if possible */
	    if (fabs(wat_nbr[np_side]) >= max_acc) {
		max_acc = fabs(wat_nbr[ct_dir]);
		r_max = dr;
		c_max = dc;
	    }
	    /* update asp */
	    if (dr != r_max || dc != c_max) {
		if (asp_val > 0) {
		    asp_val = drain[r - r_max + 1][c - c_max + 1];
		    cseg_put(&asp, &asp_val, r, c);
		}
	    }
	    is_swale = FLAG_GET(this_flag_value, SWALEFLAG);
	    /* start new stream */
	    value = fabs(value) + 0.5;
	    if (!is_swale && (int)value >= threshold && stream_cells < 4 &&
		swale_cells < 1) {
		FLAG_SET(this_flag_value, SWALEFLAG);
		is_swale = 1;
	    }
	    /* update asp for depression */
	    if (is_swale && pit_flag) {
		cseg_get(&asp, &asp_val_down, dr, dc);
		if (asp_val > 0 && asp_val_down == 0) {
		    asp_val = -asp_val;
		    cseg_put(&asp, &asp_val, r, c);
		}
	    }
	    /* continue stream */
	    if (is_swale) {
		bseg_get(&bitflags, &flag_value, r_max, c_max);
		FLAG_SET(flag_value, SWALEFLAG);
		bseg_put(&bitflags, &flag_value, r_max, c_max);
	    }
	    else {
		if (er_flag && !is_swale && !FLAG_GET(this_flag_value, RUSLEBLOCKFLAG))
		    slope_length(r, c, r_max, c_max);
	    }
	    FLAG_SET(this_flag_value, WORKEDFLAG);
	    bseg_put(&bitflags, &this_flag_value, r, c);
	}
    }
    G_percent(count, do_points, 1);	/* finish it */
    if (workedon)
	G_warning(_("MFD: A * path already processed when extracting streams: %d of %d cells"),
		  workedon, do_points);

    seg_close(&astar_pts);
    
    G_free(wat_nbr);
    G_free(ele_nbr);
    G_free(flag_nbr);

    return 0;
}
