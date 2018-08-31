#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

void adjust_window(struct Cell_head *window, int row_flag, int col_flag, int depth_flag)
{
    G_adjust_Cell_head3(window, row_flag, col_flag, depth_flag);
}
