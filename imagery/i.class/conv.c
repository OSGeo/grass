#include "globals.h"


/* conversion routines to convert from view x,y to cell col,row
 * as well as cell col,row to cell east,north
 */
int view_to_col(View * view, int x)
{
    return x - view->cell.left;
}

int view_to_row(View * view, int y)
{
    return y - view->cell.top;
}

int col_to_view(View * view, int col)
{
    return view->cell.left + col;
}

int row_to_view(View * view, int row)
{
    return view->cell.top + row;
}

/* in these next 2 routines, location determines if we are
 * converting from center of the cell (location == .5)
 * top or left edge (location == 0.0)
 * bottom or right edge (location == 1.0)
 */

double row_to_northing(struct Cell_head *cellhd, int row, double location)
{
    return cellhd->north - (row + location) * cellhd->ns_res;
}

double col_to_easting(struct Cell_head *cellhd, int col, double location)
{
    return cellhd->west + (col + location) * cellhd->ew_res;
}
