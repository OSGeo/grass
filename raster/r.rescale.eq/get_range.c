#include <grass/gis.h>
#include <grass/raster.h>

int get_range(struct Cell_stats *statf, CELL * min, CELL * max, int zero)
{
    long count;
    int any;
    CELL cat;

    any = *min = *max = 0;

    Rast_rewind_cell_stats(statf);
    while (!any && Rast_next_cell_stat(&cat, &count, statf)) {
	if (zero || cat)
	    any = 1;
    }
    if (!any)
	return 0;
    *min = *max = cat;

    while (Rast_next_cell_stat(&cat, &count, statf)) {
	if (zero || cat)
	    *max = cat;
    }
    return 1;
}
