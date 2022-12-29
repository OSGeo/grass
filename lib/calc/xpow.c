
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
pow(a,b)
   a raised to the power b
****************************************************************/

static int ipow(int x, int y)
{
    int res = 1;

    while (y) {
	if (y & 1)
	    res *= x;
	y >>= 1;
	x *= x;
    }
    return res;
}

int f_pow(int argc, const int *argt, void **args)
{
    int i;

    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    if (argt[1] != argt[0] || argt[2] != argt[0])
	return E_ARG_TYPE;

    switch (argt[0]) {
    case CELL_TYPE:
	{
	    CELL *res = args[0];
	    CELL *arg1 = args[1];
	    CELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		if (IS_NULL_C(&arg1[i]) || IS_NULL_C(&arg2[i]) || arg2[i] < 0)
		    SET_NULL_C(&res[i]);
		else
		    res[i] = ipow(arg1[i], arg2[i]);
	    }
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL *arg1 = args[1];
	    FCELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		if (IS_NULL_F(&arg1[i]) || IS_NULL_F(&arg2[i]))
		    SET_NULL_F(&res[i]);
		else if (arg1[i] < 0 && arg2[i] != ceil(arg2[i]))
		    SET_NULL_F(&res[i]);
		else {
		    floating_point_exception = 0;
		    res[i] = pow(arg1[i], arg2[i]);
		    if (floating_point_exception)
			SET_NULL_F(&res[i]);
		}
	    }
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL *arg1 = args[1];
	    DCELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		if (IS_NULL_D(&arg1[i]) || IS_NULL_D(&arg2[i]))
		    SET_NULL_D(&res[i]);
		else if (arg1[i] < 0 && arg2[i] != ceil(arg2[i]))
		    SET_NULL_D(&res[i]);
		else {
		    floating_point_exception = 0;
		    res[i] = pow(arg1[i], arg2[i]);
		    if (floating_point_exception)
			SET_NULL_D(&res[i]);
		}
	    }
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}
