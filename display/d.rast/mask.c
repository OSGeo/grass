#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "mask.h"
#include "local_proto.h"

int init_mask_rules(Mask * mask)
{
    mask->list = NULL;
    return 0;
}

int init_d_mask_rules(d_Mask * d_mask)
{
    d_mask->list = NULL;
    return 0;
}

int add_mask_rule(Mask * mask, long a, long b, int inf)
{
    Interval *I;

    I = (Interval *) G_malloc(sizeof(Interval));
    I->low = a <= b ? a : b;
    I->high = a >= b ? a : b;
    I->inf = inf;
    I->next = mask->list;
    mask->list = I;
    return 0;
}

int add_d_mask_rule(d_Mask * d_mask, double a, double b, int inf)
{
    d_Interval *I;

    I = (d_Interval *) G_malloc(sizeof(d_Interval));
    I->low = a <= b ? a : b;
    I->high = a >= b ? a : b;
    I->inf = inf;
    I->next = d_mask->list;
    d_mask->list = I;
    return 0;
}

int mask_cell_array(CELL * cell, int ncols, Mask * mask, int invert)
{
    long x;

    while (ncols-- > 0) {
	x = *cell;
	if (mask_select(&x, mask, invert))
	    *cell++ = x;
	else
	    Rast_set_c_null_value(cell++, 1);
    }
    return 0;
}

int mask_d_cell_array(DCELL * dcell, int ncols, d_Mask * mask, int invert)
{
    DCELL x;

    while (ncols-- > 0) {
	x = *dcell;
	if (mask_d_select(&x, mask, invert))
	    *dcell++ = x;
	else
	    Rast_set_d_null_value(dcell++, 1);
    }
    return 0;
}

int mask_select(long *x, Mask * mask, int invert)
{
    Interval *I;

    if (mask->list == NULL)
	return 1;
    for (I = mask->list; I; I = I->next) {
	if (mask_match_interval(*x, I)) {
	    if (invert)
		return 0;
	    return 1;
	}
    }
    return invert;
}

int mask_d_select(DCELL * x, d_Mask * mask, int invert)
{
    d_Interval *I;

    if (mask->list == NULL)
	return 1;
    for (I = mask->list; I; I = I->next) {
	if (mask_match_d_interval(*x, I)) {
	    if (invert)
		return 0;
	    return 1;
	}
    }
    return invert;
}

int mask_match_interval(long x, Interval * I)
{
    if (I->inf < 0)
	return x <= I->low;

    if (I->inf > 0)
	return x >= I->high;

    return x >= I->low && x <= I->high;
}

int mask_match_d_interval(DCELL x, d_Interval * I)
{
    if (I->inf < 0)
	return x <= I->low;

    if (I->inf > 0)
	return x >= I->high;

    return x >= I->low && x <= I->high;
}
