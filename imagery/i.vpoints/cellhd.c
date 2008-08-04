#include "globals.h"

int Outline_cellhd(View * view, struct Cell_head *cellhd)
{
    int row, col;
    int top, bottom, left, right;

    row = northing_to_row(&view->cell.head, cellhd->north) + .5;
    top = row_to_view(view, row);
    if (top < view->top)
	top = view->top;

    col = easting_to_col(&view->cell.head, cellhd->west) + .5;
    left = col_to_view(view, col);
    if (left < view->left)
	left = view->left;

    row = northing_to_row(&view->cell.head, cellhd->south) + .5;
    bottom = row_to_view(view, row);
    if (bottom > view->bottom)
	bottom = view->bottom;

    col = easting_to_col(&view->cell.head, cellhd->east) + .5;
    right = col_to_view(view, col);
    if (right > view->right)
	right = view->right;

    Outline_box(top, bottom, left, right);

    return 0;
}
