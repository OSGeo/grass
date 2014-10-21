#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int cseg_put(CSEG * cseg, CELL * value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(cseg->seg), value, row, col) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int cseg_put_row(CSEG * cseg, CELL * value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(cseg->seg), value, row) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
