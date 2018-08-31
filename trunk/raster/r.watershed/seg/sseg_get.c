#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int seg_get(SSEG * sseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(sseg->seg), value, row, col) < 0) {
	G_warning("seg_get(): could not read segment file");
	return -1;
    }
    return 0;
}

int seg_get_row(SSEG * sseg, char *value, GW_LARGE_INT row)
{
    if (Segment_get_row(&(sseg->seg), value, row) < 0) {
	G_warning("seg_get(): could not read segment file");
	return -1;
    }
    return 0;
}

int seg_flush(SSEG * sseg)
{
    Segment_flush(&(sseg->seg));
    return 0;
}
