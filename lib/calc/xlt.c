
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
lt(a,b) = a < b
****************************************************************/

int f_lt(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    int i;

    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    switch (argt[1]) {
    case CELL_TYPE:
	{
	    CELL *arg1 = args[1];
	    CELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		if (IS_NULL_C(&arg1[i]) || IS_NULL_C(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg1[i] < arg2[i];
	    }
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *arg1 = args[1];
	    FCELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		if (IS_NULL_F(&arg1[i]) || IS_NULL_F(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg1[i] < arg2[i];
	    }
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *arg1 = args[1];
	    DCELL *arg2 = args[2];

	    for (i = 0; i < columns; i++) {
		if (IS_NULL_D(&arg1[i]) || IS_NULL_D(&arg2[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = arg1[i] < arg2[i];
	    }
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}
