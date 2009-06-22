#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int cseg_get(CSEG * cseg, CELL * value, int row, int col)
{
    if (segment_get(&(cseg->seg), value, row, col) < 0) {
	G_warning("cseg_get(): could not read segment file");
	return -1;
    }
    return 0;
}
