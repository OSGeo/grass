#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int bseg_get(BSEG * bseg, CELL * value, int row, int col)
{
    CELL x;

    if (segment_get(&(bseg->seg), &x, row, col >> 3) < 0) {
	G_warning("bseg_get(): could not read segment file");
	return -1;
    }
    *value = (CELL) ((x & (1 << (col & 7))) ? 1 : 0);
    return 0;
}
