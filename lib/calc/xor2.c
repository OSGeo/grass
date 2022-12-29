
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
or2(a,b,c,...) = a || b || c || ...

Differs from or() in that the boolean axioms:

	true || x == true
	x || true == true

hold even when x is null.
****************************************************************/

int f_or2(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    int i, j;

    if (argc < 1)
	return E_ARG_LO;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    for (i = 1; i <= argc; i++)
	if (argt[i] != argt[0])
	    return E_ARG_TYPE;

    for (i = 0; i < columns; i++) {
	res[i] = 0;
	for (j = 1; j <= argc; j++) {
	    CELL *arg = args[j];
	    if (!IS_NULL_C(&arg[i]) && arg[i]) {
		res[i] = 1;
		break;
	    }
	    if (IS_NULL_C(&arg[i]))
		SET_NULL_C(&res[i]);
	}
    }

    return 0;
}
