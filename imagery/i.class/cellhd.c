#include "globals.h"
#include "local_proto.h"


int Outline_cellhd(View * view, struct Cell_head *cellhd)
{
    int row, col;
    int top, bottom, left, right;

    row = G_northing_to_row(cellhd->north, &view->cell.head) + .5;
    top = row_to_view(view, row);
    col = G_easting_to_col(cellhd->west, &view->cell.head) + .5;
    left = col_to_view(view, col);
    row = G_northing_to_row(cellhd->south, &view->cell.head) + .5;
    bottom = row_to_view(view, row);
    col = G_easting_to_col(cellhd->east, &view->cell.head) + .5;
    right = col_to_view(view, col);

    Outline_box(top, bottom, left, right);

    return 0;
}
