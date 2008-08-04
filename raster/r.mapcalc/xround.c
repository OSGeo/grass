
#include <limits.h>

#include <grass/gis.h>
#include "globals.h"
#include "expression.h"
#include "func_proto.h"

/**********************************************************************
round(x)

  rounds x to nearest integer.

  if input is CELL (which is an integer already)
  the input argument (argv[0]) is simply copied to the output cell.

  if the input is double, the input is rounded by adding .5 to positive
  numbers, and subtracting .5 from negatives.
**********************************************************************/

/* round(x) rounds x to nearest CELL value, handles negative correctly */

static int round(double x)
{
    int n;

    if (IS_NULL_D(&x))
	SET_NULL_C(&n);
    else if (x > INT_MAX || x < -INT_MAX) {
	SET_NULL_C(&n);
	if (!IS_NULL_D(&x))
	    overflow_occurred = 1;
    }
    else if (x >= 0.0)
	n = x + .5;
    else {
	n = -x + .5;
	n = -n;
    }

    return n;
}

/**********************************************************************/

int f_round(int argc, const int *argt, void **args)
{
    int *res = args[0];
    int i;

    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    if (argt[0] != CELL_TYPE)
	return E_RES_TYPE;

    switch (argt[1]) {
    case CELL_TYPE:
	{
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
	    FCELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		if (IS_NULL_F(&arg1[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = round(arg1[i]);
	    return 0;
	}
    case DCELL_TYPE:
	{
	    DCELL *arg1 = args[1];

	    for (i = 0; i < columns; i++)
		if (IS_NULL_D(&arg1[i]))
		    SET_NULL_C(&res[i]);
		else
		    res[i] = round(arg1[i]);
	    return 0;
	}
    default:
	return E_INV_TYPE;
    }
}

int c_round(int argc, int *argt)
{
    if (argc < 1)
	return E_ARG_LO;
    if (argc > 1)
	return E_ARG_HI;

    argt[0] = CELL_TYPE;
    /*      argt[1] = argt[1];      */

    return 0;
}
