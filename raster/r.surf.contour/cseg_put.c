#include <grass/gis.h>
#include "cseg.h"

int cseg_put(CSEG * cseg, int row, int col, CELL value)
{
    if (segment_put(&(cseg->seg), &value, row, col) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
