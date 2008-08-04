#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int cseg_put(CSEG * cseg, CELL * value, int row, int col)
{
    if (segment_put(&(cseg->seg), value, row, col) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
