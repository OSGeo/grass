#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int cseg_get(CSEG * cseg, CELL * value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(cseg->seg), value, row, col) < 0) {
	G_warning("cseg_get(): could not read segment file");
	return -1;
    }
    return 0;
}
