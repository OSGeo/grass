#include <grass/gis.h>
#include "cseg.h"

int cseg_get(CSEG * cseg, int row, int col, CELL * value)
{
    if (segment_get(&(cseg->seg), value, row, col) < 0) {
	G_warning("cseg_get(): could not read segment file");
	return -1;
    }
    return 0;
}
