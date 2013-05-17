
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/****************************************************************
mod(a,b) = a % b
****************************************************************/

int f_mod(int argc, const int *argt, void **args)
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
		if (IS_NULL_C(&arg1[i]) || IS_NULL_C(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg1[i] % arg2[i];
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
		else {
		    floating_point_exception = 0;
		    res[i] = (FCELL) fmod(arg1[i], arg2[i]);
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
		else {
		    floating_point_exception = 0;
		    res[i] = (DCELL) fmod(arg1[i], arg2[i]);
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
