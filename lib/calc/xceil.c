
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/**********************************************************************
ceil(x)

   the smallest integral value that is not less than x
**********************************************************************/

int f_ceil(int argc, const int *argt, void **args)
{
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    if (argt[0] != argt[1])
	return E_RES_TYPE;

    switch (argt[1]) {
    case CELL_TYPE:
	{
	    CELL *res = args[0];
	    CELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		if (IS_NULL_C(&arg1[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg1[i];
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		if (IS_NULL_F(&arg1[i]))
		    SET_NULL_F(&res[i]);
		else
		    res[i] = (FCELL) ceil(arg1[i]);
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		if (IS_NULL_D(&arg1[i]))
		    SET_NULL_D(&res[i]);
		else
		    res[i] = ceil(arg1[i]);
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}
