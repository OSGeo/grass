#include "contour.h"

NODE *add_in(int r, int c, int rr, int cc, NODE * zero, int *node_ct)
{
    int dor, doc;

    FLAG_SET(seen, rr, cc);
    if (rr < minr)
	minr = rr;
    if (rr > maxr)
	maxr = rr;
    if (cc < minc)
	minc = cc;
    if (cc > maxc)
	maxc = cc;
    if (*node_ct == array_size) {
	zero = (NODE *) G_realloc(zero, (array_size + AR_INCR) *
				  sizeof(NODE));
	array_size += AR_INCR;
    }
    dor = ABS(rr - r);
    doc = ABS(cc - c);
    zero[*node_ct].r = rr;
    zero[*node_ct].c = cc;
    zero[*node_ct].d = MIN(dor, doc) * 1.414 + ABS(dor - doc);
    *node_ct = *node_ct + 1;
    return (zero);
}
