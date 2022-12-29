
#include <grass/gis.h>
#include <grass/raster3d.h>
#include "globals.h"
#include "globals3.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
x() easting at center of column
y() northing at center of row
z() height at center of depth
**********************************************************************/

int f_x(int argc, const int *argt, void **args)
{
    RASTER3D_Region *window = &current_region3;
    DCELL *res = args[0];
    DCELL x;
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    x = window->west + 0.5 * window->ew_res;

    for (i = 0; i < columns; i++) {
	res[i] = x;
	x += window->ew_res;
    }

    return 0;
}

int f_y(int argc, const int *argt, void **args)
{
    RASTER3D_Region *window = &current_region3;
    DCELL *res = args[0];
    DCELL y;
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    y = window->north - (current_row + 0.5) * window->ns_res;

    for (i = 0; i < columns; i++)
	res[i] = y;

    return 0;
}

int f_z(int argc, const int *argt, void **args)
{
    RASTER3D_Region *window = &current_region3;
    DCELL *res = args[0];
    DCELL z;
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    z = window->bottom + ((current_depth) + 0.5) * window->tb_res;

    for (i = 0; i < columns; i++)
	res[i] = z;

    return 0;
}
