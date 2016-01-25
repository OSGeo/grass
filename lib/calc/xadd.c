
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
add(a,b,c,...) = a + b + c + ...
****************************************************************/

int f_add(int argc, const int *argt, void **args)
{
    int i, j;

    if (argc < 1)
	return E_ARG_LO;

    for (i = 1; i <= argc; i++)
	if (argt[i] != argt[0])
	    return E_ARG_TYPE;

    switch (argt[0]) {
    case CELL_TYPE:
	{
	    CELL *res = args[0];
	    CELL **argz = (CELL **) args;

	    for (i = 0; i < columns; i++) {
		res[i] = 0;
		for (j = 1; j <= argc; j++) {
		    if (IS_NULL_C(&argz[j][i])) {
			SET_NULL_C(&res[i]);
			break;
		    }
		    res[i] += argz[j][i];
		}
	    }
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL **argz = (FCELL **) args;

	    for (i = 0; i < columns; i++) {
		res[i] = 0;
		for (j = 1; j <= argc; j++) {
		    if (IS_NULL_F(&argz[j][i])) {
			SET_NULL_F(&res[i]);
			break;
		    }
		    res[i] += argz[j][i];
		}
	    }
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL **argz = (DCELL **) args;

	    for (i = 0; i < columns; i++) {
		res[i] = 0;
		for (j = 1; j <= argc; j++) {
		    if (IS_NULL_D(&argz[j][i])) {
			SET_NULL_D(&res[i]);
			break;
		    }
		    res[i] += argz[j][i];
		}
	    }
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}
