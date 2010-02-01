#include "local_proto.h"


int adjcellhd(struct Cell_head *cellhd)
{
    int retval = 0;

    G_set_window(cellhd);

    if (cellhd->rows != Rast_window_rows())
	retval = 2;

    if (cellhd->cols != Rast_window_cols())
	retval = 3;

    return (retval);
}
