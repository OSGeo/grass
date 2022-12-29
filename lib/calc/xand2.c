
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/calc.h>

/****************************************************************
and2(a,b,c,...) = a && b && c && ...

Differs from and() in that the boolean axioms:

	false && x == false
	x && false == false

hold even when x is null.
****************************************************************/

int f_and2(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    CELL **argz = (CELL **) args;
    int i, j;

    if (argc < 1)
	return E_ARG_LO;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    for (i = 1; i <= argc; i++)
	if (argt[i] != CELL_TYPE)
	    return E_ARG_TYPE;

    for (i = 0; i < columns; i++) {
	res[i] = 1;
	for (j = 1; j <= argc; j++) {
	    if (!IS_NULL_C(&argz[j][i]) && !argz[j][i]) {
		res[i] = 0;
		break;
	    }
	    if (IS_NULL_C(&argz[j][i]))
		SET_NULL_C(&res[i]);
	}
    }

    return 0;
}
