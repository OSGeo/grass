#include "contour.h"

NODE *addpts_slow(NODE * zero, int r, int c, int rr, int cc, int *node_ct)
{
    NODE *add_in_slow();
    CELL value;

    if (rr < nrows - 1) {
	bseg_get(&bseen, &value, rr + 1, cc);
	if (!value)
	    zero = add_in_slow(r, c, rr + 1, cc, zero, node_ct);
    }
    if (cc < ncols - 1) {
	bseg_get(&bseen, &value, rr, cc + 1);
	if (!value)
	    zero = add_in_slow(r, c, rr, cc + 1, zero, node_ct);
    }
    if (rr > 0) {
	bseg_get(&bseen, &value, rr - 1, cc);
	if (!value)
	    zero = add_in_slow(r, c, rr - 1, cc, zero, node_ct);
    }
    if (cc > 0) {
	bseg_get(&bseen, &value, rr, cc - 1);
	if (!value)
	    zero = add_in_slow(r, c, rr, cc - 1, zero, node_ct);
    }
    return (zero);
}

NODE *addpts(NODE * zero, int r, int c, int rr, int cc, int *node_ct)
{
    NODE *add_in();

    if (rr < nrows - 1) {
	if (!flag_get(seen, rr + 1, cc))
	    zero = add_in(r, c, rr + 1, cc, zero, node_ct);
    }
    if (cc < ncols - 1) {
	if (!flag_get(seen, rr, cc + 1))
	    zero = add_in(r, c, rr, cc + 1, zero, node_ct);
    }
    if (rr > 0) {
	if (!flag_get(seen, rr - 1, cc))
	    zero = add_in(r, c, rr - 1, cc, zero, node_ct);
    }
    if (cc > 0) {
	if (!flag_get(seen, rr, cc - 1))
	    zero = add_in(r, c, rr, cc - 1, zero, node_ct);
    }
    return (zero);
}
