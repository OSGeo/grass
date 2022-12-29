#include "Gwater.h"

int slope_length(int r, int c, int dr, int dc)
{
    CELL top_alt, bot_alt;
    double res, top_ls, bot_ls;

    if (r == dr)
	res = window.ew_res;
    else if (c == dc)
	res = window.ns_res;
    else
	res = diag;

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
