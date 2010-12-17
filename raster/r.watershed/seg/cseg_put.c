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

int cseg_put_row(CSEG * cseg, CELL * value, int row)
{
    if (segment_put_row(&(cseg->seg), value, row) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}
