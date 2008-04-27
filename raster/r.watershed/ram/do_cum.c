#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int do_cum (void)
{
    SHORT	r, c, dr, dc;
    CELL	is_swale, value, valued;
    int		killer, threshold, count;

    G_message(_("SECTION 3: Accumulating Surface Flow."));

count = 0;
if (bas_thres <= 0) threshold = 60;
else	threshold = bas_thres;
while (first_cum != -1) {
	G_percent (count++, do_points, 3);
	killer = first_cum;
	first_cum = astar_pts[killer].nxt;
        if ((dr = astar_pts[killer].downr) > -1) { 
		r = astar_pts[killer].r;
                c = astar_pts[killer].c;
                dc = astar_pts[killer].downc;
		value = wat[SEG_INDEX(wat_seg, r, c)];
		if (ABS(value) >= threshold)
			FLAG_SET(swale, r, c);
		valued = wat[SEG_INDEX(wat_seg, dr, dc)];
                if (value > 0) {
			if (valued > 0)
                    		valued += value;
			else
                    		valued -= value;
		} else {
			if (valued < 0)
                    		valued += value;
			else
                    		valued = value - valued;
                }
		wat[SEG_INDEX(wat_seg, dr, dc)] = valued;
		is_swale = FLAG_GET(swale, r, c);
		if (is_swale || ABS(valued) >= threshold) {
			FLAG_SET(swale, dr, dc);
		} else {
			if (er_flag && !is_swale)
				slope_length (r, c, dr, dc);
		}
        }
}
G_percent (count, do_points, 3); /* finish it */
G_free (astar_pts);

return 0;
}
