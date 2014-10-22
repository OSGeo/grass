#include <grass/gis.h>
#include <grass/segment.h>
#include "Gwater.h"

int bseg_put(BSEG * bseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(bseg->seg), value, row, col) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int bseg_put_row(BSEG * bseg, char *value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(bseg->seg), value, row) < 0) {
	G_warning("cseg_put(): could not write segment file");
	return -1;
    }
    return 0;
}

int bseg_put_old(BSEG * bseg, CELL * value, int row, int col)
{
    CELL old_value;

    if (Segment_get(&(bseg->seg), &old_value, row, col >> 3) < 0) {
	G_warning("bseg_put(): could not read segment file");
	return -1;
    }
    if (*value)
	old_value |= (1 << (col & 7));
    else
	old_value &= ~(1 << (col & 7));
    if (Segment_put(&(bseg->seg), &old_value, row, col >> 3) < 0) {
	G_warning("bseg_put(): could not write segment file");
	return -2;
    }
    return 0;
}
