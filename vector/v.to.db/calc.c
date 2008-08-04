#include <grass/gis.h>

double length(register int np, register double *x, register double *y)
{
    register double len;
    register int i;

    len = 0.0;
    for (i = 0; i < (np - 1); i++)
	len += G_distance(*(x + i), *(y + i), *(x + i + 1), *(y + i + 1));

    return (len);
}
