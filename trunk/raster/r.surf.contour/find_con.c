#include <math.h>
#include "contour.h"

int find_con(int r, int c, double *d1, double *d2, DCELL * con1, DCELL * con2)
{
    int ct, low_ct, node_ct;
    int rr, cc, dor, doc;
    double dd, shortest;
    DCELL value;

    Rast_set_d_null_value(con1, 1);
    Rast_set_d_null_value(con2, 1);
    *d1 = *d2 = 1.0;
    shortest = nrows * ncols;
    for (rr = minr; rr <= maxr; rr++) {
	for (cc = minc; cc <= maxc; cc++)
	    FLAG_UNSET(seen, rr, cc);
    }
    minr = nrows;
    minc = ncols;
    maxr = maxc = -1;
    FLAG_SET(seen, r, c);
    if (r < minr)
	minr = r;
    if (r > maxr)
	maxr = r;
    if (c < minc)
	minc = c;
    if (c > maxc)
	maxc = c;
    node_ct = 0;
    zero = addpts(zero, r, c, r, c, &node_ct);
    low_ct = 0;
    while (1) {
	ct = low_ct++;
	if (node_ct <= ct)
	    return 1;
	rr = zero[ct].r;
	cc = zero[ct].c;
	dor = ABS(rr - r);
	doc = ABS(cc - c);
	if (rr >= 0 && cc >= 0 && rr < nrows && cc < ncols
	    && zero[ct].d < shortest && !flag_get(mask, rr, cc)) {
	    value = con[rr][cc];
	    if (Rast_is_d_null_value(&value))
		zero = addpts(zero, r, c, rr, cc, &node_ct);
	    else if (Rast_is_d_null_value(con1)) {
		*con1 = value;
		*d1 = MIN(dor, doc) * 1.414 + ABS(dor - doc);
		shortest = *d1 * 2.0 * i_val_l_f;
	    }
	    else if (*con1 == value) {
		dd = MIN(dor, doc) * 1.414 + ABS(dor - doc);
		if (dd < *d1) {
		    *d1 = dd;
		    shortest = dd * 2.0 * i_val_l_f;
		}
	    }
	    else if (Rast_is_d_null_value(con2)) {
		*con2 = value;
		*d2 = MIN(dor, doc) * 1.414 + ABS(dor - doc);
		shortest = *d2;
	    }
	    else {
		dd = MIN(dor, doc) * 1.414 + ABS(dor - doc);
		shortest = MIN(shortest, dd);
	    }
	}
    }

    return 0;
}
