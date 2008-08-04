
/**************************************************************
* G_histogram_eq (histo, map, min, max)
*
*   struct Histogram *histo;    histogram as returned by G_read_histogram()
*   unsigned char **map;        equalized category mapping
*   CELL *min, *max;            min,max category for map
*
* perform histogram equalization
* inputs are histo, output is map,min,max
****************************************************************/
#include <grass/gis.h>
int G_histogram_eq(const struct Histogram *histo,
		   unsigned char **map, CELL * min, CELL * max)
{
    int i;
    int x;
    CELL cat, prev;
    double total;
    double sum;
    double span;
    int ncats;
    long count;
    unsigned char *xmap;
    int len;
    int first, last;

    ncats = G_get_histogram_num(histo);
    if (ncats == 1) {
	*min = *max = G_get_histogram_cat(0, histo);
	*map = xmap = (unsigned char *)G_malloc(1);
	*xmap = 0;
	return 0;
    }
    if ((*min = G_get_histogram_cat(first = 0, histo)) == 0)
	*min = G_get_histogram_cat(++first, histo);
    if ((*max = G_get_histogram_cat(last = ncats - 1, histo)) == 0)
	*max = G_get_histogram_cat(--last, histo);
    len = *max - *min + 1;
    *map = xmap = (unsigned char *)G_malloc(len);

    total = 0;
    for (i = first; i <= last; i++) {
	if (G_get_histogram_cat(i, histo) == 0)
	    continue;
	count = G_get_histogram_count(i, histo);
	if (count > 0)
	    total += count;
    }
    if (total <= 0) {
	for (i = 0; i < len; i++)
	    xmap[i] = 0;
	return 0;
    }

    span = total / 256;

    sum = 0.0;
    cat = *min - 1;
    for (i = first; i <= last; i++) {
	prev = cat + 1;
	cat = G_get_histogram_cat(i, histo);
	count = G_get_histogram_count(i, histo);
	if (count < 0 || cat == 0)
	    count = 0;
	x = (sum + (count / 2.0)) / span;
	if (x < 0)
	    x = 0;
	else if (x > 255)
	    x = 255;
	sum += count;

	while (prev++ <= cat)
	    *xmap++ = x;
    }

    return 0;
}
