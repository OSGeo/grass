
#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "globals2.h"
#include "expression.h"
#include "func_proto.h"

/****************************************************************
ewres() east-west resolution
nsres() north-south resolution
tbres() top-bottom resolution
****************************************************************/

int f_ewres(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    for (i = 0; i < columns; i++)
	res[i] = current_region2.ew_res;

    return 0;
}

int f_nsres(int argc, const int *argt, void **args)
{
    DCELL *res = args[0];
    int i;

    if (argc > 0)
	return E_ARG_HI;

    if (argt[0] != DCELL_TYPE)
	return E_RES_TYPE;

    for (i = 0; i < columns; i++)
	res[i] = current_region2.ns_res;

    return 0;
}

int f_tbres(int argc, const int *argt, void **args)
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
