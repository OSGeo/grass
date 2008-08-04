#include "defs.h"

int edge_order(const void *aa, const void *bb)
{
    const POINT *a = aa;
    const POINT *b = bb;

    if (a->y < b->y)
	return (-1);
    if (a->y > b->y)
	return (1);

    if (a->x < b->x)
	return (-1);
    if (a->x > b->x)
	return (1);

    return (0);
}
