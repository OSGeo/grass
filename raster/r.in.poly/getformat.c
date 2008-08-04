#include <grass/gis.h>
#include "format.h"

int getformat(FILE * fd)
{
    char buf[1024];
    long x;
    CELL max, min, cat;
    unsigned char Umin, Umax;
    char Cmin, Cmax;
    short Smin, Smax;
    int first;

    max = min = 0;
    first = 1;
    fseek(fd, 0L, 0);
    while (G_getl2(buf, (sizeof buf) - 1, fd)) {
	G_strip(buf);
	if (*buf != '=')
	    continue;
	if (sscanf(buf + 1, "%ld", &x) != 1)
	    continue;
	cat = (CELL) x;
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
