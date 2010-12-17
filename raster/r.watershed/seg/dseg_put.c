#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int dseg_put(DSEG * dseg, double *value, int row, int col)
{
    if (segment_put(&(dseg->seg), (DCELL *) value, row, col) < 0) {
	G_warning("dseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int dseg_put_row(DSEG * dseg, double *value, int row)
{
    if (segment_put_row(&(dseg->seg), (DCELL *) value, row) < 0) {
	G_warning("dseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
