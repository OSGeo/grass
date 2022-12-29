
/**************************************************************
* Rast_histogram_eq (histo, map, min, max)
*
*   struct Histogram *histo;    histogram as returned by Rast_read_histogram()
*   unsigned char **map;        equalized category mapping
*   CELL *min, *max;            min,max category for map
*
* perform histogram equalization
* inputs are histo, output is map,min,max
****************************************************************/
#include <grass/gis.h>
#include <grass/raster.h>

void Rast_histogram_eq(const struct Histogram *histo,
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

    ncats = Rast_get_histogram_num(histo);
    if (ncats == 1) {
	*min = *max = Rast_get_histogram_cat(0, histo);
	*map = xmap = (unsigned char *)G_malloc(1);
	*xmap = 0;
	return;
    }
    if ((*min = Rast_get_histogram_cat(first = 0, histo)) == 0)
	*min = Rast_get_histogram_cat(++first, histo);
    if ((*max = Rast_get_histogram_cat(last = ncats - 1, histo)) == 0)
	*max = Rast_get_histogram_cat(--last, histo);
    len = *max - *min + 1;
    *map = xmap = (unsigned char *)G_malloc(len);

    total = 0;
    for (i = first; i <= last; i++) {
	if (Rast_get_histogram_cat(i, histo) == 0)
	    continue;
	count = Rast_get_histogram_count(i, histo);
	if (count > 0)
	    total += count;
    }
    if (total <= 0) {
	for (i = 0; i < len; i++)
	    xmap[i] = 0;
	return;
    }

    span = total / 256;

    sum = 0.0;
    cat = *min - 1;
    for (i = first; i <= last; i++) {
	prev = cat + 1;
	cat = Rast_get_histogram_cat(i, histo);
	count = Rast_get_histogram_count(i, histo);
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
}
