
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
not(a) = !a
****************************************************************/

int f_not(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    CELL *arg1 = args[1];
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    if (argt[1] != CELL_TYPE)
	return E_ARG_TYPE;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    for (i = 0; i < columns; i++) {
	if (IS_NULL_C(&arg1[i]))
	    SET_NULL_C(&res[i]);
	else
	    res[i] = !arg1[i];
    }

    return 0;
}

int c_not(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    if (argt[1] != CELL_TYPE)
	return E_ARG_TYPE;

    argt[0] = CELL_TYPE;

    return 0;
}
