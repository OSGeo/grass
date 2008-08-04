#include <grass/gis.h>
#include <stdio.h>
#include <stdlib.h>
#include "includes.h"

XPoint *alloc_xpoints(int count)
{
    static size_t num_alloc;
    static XPoint *pnts;

    if (num_alloc < count) {
	num_alloc = count;
	pnts = G_realloc(pnts, num_alloc * sizeof(XPoint));
    }

    return pnts;
}
