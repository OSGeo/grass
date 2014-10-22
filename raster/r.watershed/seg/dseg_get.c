#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int dseg_get(DSEG * dseg, double *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(dseg->seg), (DCELL *) value, row, col) < 0) {
	G_warning("dseg_get(): could not read segment file");
	return -1;
    }
    return 0;
}

int dseg_flush(DSEG * dseg)
{
    Segment_flush(&(dseg->seg));
    return 0;
}
