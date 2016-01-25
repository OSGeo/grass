
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
max(x0,x1,...,xn) returns maximum value
****************************************************************/

int f_nmax(int argc, const int *argt, void **args)
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
		int nul = 1;
		CELL max;

		for (j = 1; j <= argc; j++)
		    if (IS_NULL_C(&argz[j][i]))
			continue;
		    else if (nul)
			max = argz[j][i], nul = 0;
		    else if (max < argz[j][i])
			max = argz[j][i], nul = 0;
		if (nul)
		    SET_NULL_C(&res[i]);
		else
		    res[i] = max;
	    }
	    return 0;
	}
    case FCELL_TYPE:
	{
	    FCELL *res = args[0];
	    FCELL **argz = (FCELL **) args;

	    for (i = 0; i < columns; i++) {
		int nul = 1;
		FCELL max;

		for (j = 1; j <= argc; j++)
		    if (IS_NULL_F(&argz[j][i]))
			continue;
		    else if (nul)
			max = argz[j][i], nul = 0;
		    else if (max < argz[j][i])
			max = argz[j][i], nul = 0;
		if (nul)
		    SET_NULL_F(&res[i]);
		else
		    res[i] = max;
	    }

	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *res = args[0];
	    DCELL **argz = (DCELL **) args;

	    for (i = 0; i < columns; i++) {
		int nul = 1;
		DCELL max;

		for (j = 1; j <= argc; j++)
		    if (IS_NULL_D(&argz[j][i]))
			continue;
		    else if (nul)
			max = argz[j][i], nul = 0;
		    else if (max < argz[j][i])
			max = argz[j][i], nul = 0;
		if (nul)
		    SET_NULL_D(&res[i]);
		else
		    res[i] = max;
	    }

	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}
