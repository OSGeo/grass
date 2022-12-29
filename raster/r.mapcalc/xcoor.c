
#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "globals2.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
x() easting at center of column
y() northing at center of row
z() height at center of depth
**********************************************************************/

int f_x(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    DCELL x;
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    x = Rast_col_to_easting(0.5, &current_region2);

    for (i = 0; i < columns; i++) {
	res[i] = x;
	x += current_region2.ew_res;
    }

    return 0;
}

int f_y(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    DCELL y;
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    y = Rast_row_to_northing(current_row + 0.5, &current_region2);

    for (i = 0; i < columns; i++)
	res[i] = y;

    return 0;
}

int f_z(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    for (i = 0; i < columns; i++)
	SET_NULL_D(&res[i]);

    return 0;
}
