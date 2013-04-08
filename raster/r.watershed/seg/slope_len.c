#include "Gwater.h"

int slope_length(int r, int c, int dr, int dc)
{
    CELL top_alt, bot_alt, ridge;
    char asp_value;
    double res, top_ls, bot_ls;
    WAT_ALT wa;

    if (r == dr)
	res = window.ns_res;
    else if (c == dc)
	res = window.ew_res;
    else
	res = diag;

    dseg_get(&s_l, &top_ls, r, c);
    if (top_ls == half_res)
	top_ls = res;
    else
	top_ls += res;
    dseg_put(&s_l, &top_ls, r, c);
    seg_get(&watalt, (char *) &wa, r, c);
    top_alt = wa.ele;
    seg_get(&watalt, (char *) &wa, dr, dc);
    bot_alt = wa.ele;
    if (top_alt > bot_alt) {
	dseg_get(&s_l, &bot_ls, dr, dc);
	if (top_ls > bot_ls) {
	    bot_ls = top_ls + res;
	    dseg_put(&s_l, &bot_ls, dr, dc);
	    cseg_get(&r_h, &ridge, r, c);
	    cseg_put(&r_h, &ridge, dr, dc);
	}
    }

    return 0;
}
