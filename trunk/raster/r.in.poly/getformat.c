#include <grass/gis.h>
#include <grass/raster.h>
#include "format.h"

int getformat(FILE * fd, int raster_type, int *null)
{
    char buf[1024];
    long x;
    CELL max, min, cat;
    unsigned char Umin, Umax;
    char Cmin, Cmax;
    short Smin, Smax;
    int first;

    if (raster_type == FCELL_TYPE)
        return USE_FCELL;
    if (raster_type == DCELL_TYPE)
        return USE_DCELL;

    if (null)
        return USE_CELL;

    max = min = 0;
    first = 1;
    G_fseek(fd, 0L, 0);
    while (G_getl2(buf, (sizeof buf) - 1, fd)) {
	G_strip(buf);
	if (*buf != '=')
	    continue;
	if (sscanf(buf + 1, "%ld", &x) != 1)
	    continue;
	cat = (CELL) x;
	/* if we want to write zeros, we must use CELL */
	if (cat == 0)
	    return USE_CELL;
	if (first) {
	    first = 0;
	    max = cat;
	    min = cat;
	}
	else if (cat > max)
	    max = cat;
	else if (cat < min)
	    min = cat;
    }

    /* test char */
    Cmax = (char)max;
    Cmin = (char)min;
    if (Cmin == min && Cmax == max)
	return (USE_CHAR);

    /* test unsigned char */
    Umax = (unsigned char)max;
    Umin = (unsigned char)min;
    if (Umin == min && Umax == max)
	return (USE_UCHAR);

    /* test short */
    Smax = (short)max;
    Smin = (short)min;
    if (Smin == min && Smax == max)
	return (USE_SHORT);

    return (USE_CELL);
}
