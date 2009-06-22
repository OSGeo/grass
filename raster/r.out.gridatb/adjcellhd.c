#include "local_proto.h"


int adjcellhd(struct Cell_head *cellhd)
{
    int retval = 0;

    if (G_set_window(cellhd) < 0)
	retval = 1;

    if (cellhd->rows != G_window_rows())
	retval = 2;

    if (cellhd->cols != G_window_cols())
	retval = 3;

    return (retval);
}
