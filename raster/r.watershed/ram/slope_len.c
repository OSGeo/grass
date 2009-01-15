#include "Gwater.h"

int slope_length(SHORT r, SHORT c, SHORT dr, SHORT dc)
{
    CELL top_alt, bot_alt, asp_value;
    double res, top_ls, bot_ls;

    if (sides == 8) {
	if (r == dr)
	    res = window.ns_res;
	else if (c == dc)
	    res = window.ew_res;
	else
	    res = diag;
    }
    else {			/* sides == 4 */

	asp_value = asp[SEG_INDEX(asp_seg, dr, dc)];
	if (r == dr) {
	    if (asp_value == 2 || asp_value == 6)
		res = window.ns_res;
	    else		/* asp_value == 4, 8, -2, -4, -6, or -8 */
		res = diag;     /* how can res be diag with sides == 4??? */
	}
	else {			/* c == dc */

	    if (asp_value == 4 || asp_value == 8)
		res = window.ew_res;
	    else		/* asp_value == 2, 6, -2, -4, -6, or -8 */
		res = diag;
	}
    }
    top_ls = s_l[SEG_INDEX(s_l_seg, r, c)];
    if (top_ls == half_res)
	top_ls = res;
    else
	top_ls += res;
    s_l[SEG_INDEX(s_l_seg, r, c)] = top_ls;
    top_alt = alt[SEG_INDEX(alt_seg, r, c)];
    bot_alt = alt[SEG_INDEX(alt_seg, dr, dc)];
    if (top_alt > bot_alt) {
	bot_ls = s_l[SEG_INDEX(s_l_seg, dr, dc)];
	if (top_ls > bot_ls) {
	    bot_ls = top_ls + res;
	    s_l[SEG_INDEX(s_l_seg, dr, dc)] = bot_ls;
	    r_h[SEG_INDEX(r_h_seg, dr, dc)] = r_h[SEG_INDEX(s_l_seg, r, c)];
	}
    }

    return 0;
}
