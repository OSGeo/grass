#include <grass/gis.h>
#include <grass/raster.h>

int Rast_cell_stats_histo_eq(struct Cell_stats *statf, CELL min1, CELL max1,	/* input range to be rescaled */
			     CELL min2, CELL max2,	/* output range */
			     int zero,	/* include zero if min1 <= 0 <= min2 ? */
			     void (*func) (CELL, CELL, CELL))
{
    long count, total;
    CELL prev = 0;
    CELL cat;
    CELL x;
    CELL newcat = 0;
    int first;
    double span, sum;
    double range2;


    if (min1 > max1 || min2 > max2)
	return 0;

    total = 0;
    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf)) {
	if (cat < min1)
	    continue;
	if (cat > max1)
	    break;
	if (cat == 0 && !zero)
	    continue;

	total += count;
    }
    if (total <= 0)
	return 0;

    range2 = max2 - min2 + 1;
    span = total / range2;

    first = 1;
    sum = 0;

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf)) {
	if (cat < min1)
	    continue;
	if (cat > max1)
	    break;
	if (cat == 0 && !zero)
	    continue;

	x = (sum + (count / 2.0)) / span;
	if (x < 0)
	    x = 0;
	x += min2;
	sum += count;

	if (first) {
	    prev = cat;
	    newcat = x;
	    first = 0;
	}
	else if (newcat != x) {
	    func(prev, cat - 1, newcat);
	    newcat = x;
	    prev = cat;
	}
    }
    if (!first) {
	func(prev, cat, newcat);
	if (!zero && min1 <= 0 && max1 >= 0)
	    func((CELL) 0, (CELL) 0, (CELL) 0);
    }

    return first == 0;
}
