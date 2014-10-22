#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int dseg_put(DSEG * dseg, double *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(dseg->seg), (DCELL *) value, row, col) < 0) {
	G_warning("dseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int dseg_put_row(DSEG * dseg, double *value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(dseg->seg), (DCELL *) value, row) < 0) {
	G_warning("dseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
