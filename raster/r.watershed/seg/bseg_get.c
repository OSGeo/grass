#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int bseg_get(BSEG * bseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(bseg->seg), value, row, col) < 0) {
	G_warning("cseg_get(): could not read segment file");
	return -1;
    }
    return 0;
}

int bseg_get_old(BSEG * bseg, CELL * value, int row, int col)
{
    CELL x;

    if (Segment_get(&(bseg->seg), &x, row, col >> 3) < 0) {
	G_warning("bseg_get(): could not read segment file");
	return -1;
    }
    *value = (CELL) ((x & (1 << (col & 7))) ? 1 : 0);
    return 0;
}
