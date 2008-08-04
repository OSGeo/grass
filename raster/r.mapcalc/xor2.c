
#include <grass/gis.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/****************************************************************
or2(a,b) = a || b

Differs from or() in that the boolean axioms:

	true || x == true
	x || true == true

hold even when x is null.
****************************************************************/

int f_or2(int argc, const int *argt, void **args)
{
    CELL *res = args[0];
    CELL *arg1 = args[1];
    CELL *arg2 = args[2];
    int i;

    if (argc < 2)
	return E_ARG_LO;
    if (argc > 2)
	return E_ARG_HI;

    if (argt[1] != CELL_TYPE || argt[2] != CELL_TYPE)
	return E_ARG_TYPE;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    for (i = 0; i < columns; i++) {
	if (!IS_NULL_C(&arg1[i]) && arg1[i])
	    res[i] = 1;
	else if (!IS_NULL_C(&arg2[i]) && arg2[i])
	    res[i] = 1;
	else if (IS_NULL_C(&arg1[i]) || IS_NULL_C(&arg2[i]))
	    SET_NULL_C(&res[i]);
	else
	    res[i] = 0;
    }

    return 0;
}
