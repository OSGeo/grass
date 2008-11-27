#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

int bseg_put(BSEG * bseg, CELL * value, int row, int col)
{
    CELL old_value;

    if (segment_get(&(bseg->seg), &old_value, row, col >> 3) < 0) {
	G_warning("bseg_put(): could not read segment file");
	return -1;
    }
    if (*value)
	old_value |= (1 << (col & 7));
    else
	old_value &= ~(1 << (col & 7));
    if (segment_put(&(bseg->seg), &old_value, row, col >> 3) < 0) {
	G_warning("bseg_put(): could not write segment file");
	return -2;
    }
    return 0;
}
