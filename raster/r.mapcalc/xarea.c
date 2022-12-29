
#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
area() area of a cell in square meters
**********************************************************************/

int f_area(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    int i;
    static int row = -1;
    static double cell_area = 0;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    if (row != current_row) {
	if (row == -1)
	    G_begin_cell_area_calculations();

	row = current_row;
	cell_area = G_area_of_cell_at_row(row);
    }

    for (i = 0; i < columns; i++)
	res[i] = cell_area;

    return 0;
}
