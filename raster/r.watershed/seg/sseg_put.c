#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int seg_put(SSEG * sseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(sseg->seg), value, row, col) < 0) {
	G_warning("seg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int seg_put_row(SSEG * sseg, char *value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(sseg->seg), value, row) < 0) {
	G_warning("seg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
